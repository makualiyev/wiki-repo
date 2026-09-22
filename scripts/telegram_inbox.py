#!/usr/bin/env python3
"""Pull new messages from the Telegram INBOX topic into INBOX.md.

Polls the Telegram Bot API (getUpdates) for messages posted in a single
forum topic of a private group, sorts them into INBOX.md's sections based
on hashtags / content, and writes the result back in place. Offset state
(which updates have already been consumed) is kept in a small JSON file so
re-runs don't reprocess old messages.

Config comes from environment variables:
  TELEGRAM_BOT_TOKEN        bot token from @BotFather
  TELEGRAM_CHAT_ID          numeric id of the group (e.g. -1001234567890)
  TELEGRAM_INBOX_THREAD_ID  message_thread_id of the "INBOX" topic
"""
from __future__ import annotations

import json
import os
import re
import sys
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
INBOX_PATH = REPO_ROOT / "INBOX.md"
STATE_PATH = REPO_ROOT / ".github" / "telegram-state.json"

SECTION_UNSORTED = "## Unsorted"
SECTION_TIL = "## To turn into a TIL"
SECTION_BOOKMARKS = "## Bookmarks to file"

URL_RE = re.compile(r"https?://\S+")
HASHTAG_RE = re.compile(r"#(\w+)")
API_TIMEOUT = 20


def api_call(token: str, method: str, params: dict) -> dict:
    url = f"https://api.telegram.org/bot{token}/{method}"
    data = urllib.parse.urlencode(params).encode()
    req = urllib.request.Request(url, data=data)
    try:
        with urllib.request.urlopen(req, timeout=API_TIMEOUT) as resp:
            body = json.load(resp)
    except urllib.error.HTTPError as e:
        raise RuntimeError(f"Telegram API {method} failed: {e.read().decode()}") from e
    if not body.get("ok"):
        raise RuntimeError(f"Telegram API {method} returned error: {body}")
    return body["result"]


def load_state() -> dict:
    if STATE_PATH.exists():
        return json.loads(STATE_PATH.read_text())
    return {"offset": 0}


def save_state(state: dict) -> None:
    STATE_PATH.parent.mkdir(parents=True, exist_ok=True)
    STATE_PATH.write_text(json.dumps(state, indent=2) + "\n")


def message_link(chat_id: int, thread_id: int, message_id: int) -> str:
    short_id = str(chat_id)
    if short_id.startswith("-100"):
        short_id = short_id[4:]
    return f"https://t.me/c/{short_id}/{thread_id}/{message_id}"


def format_entry(message: dict, chat_id: int, thread_id: int) -> tuple[str, str]:
    """Return (section_heading, markdown_bullet) for one Telegram message."""
    text = message.get("text") or message.get("caption") or ""
    tags = {t.lower() for t in HASHTAG_RE.findall(text)}
    cleaned = HASHTAG_RE.sub("", text).strip()
    cleaned = re.sub(r"\s+", " ", cleaned).strip()
    urls = URL_RE.findall(text)

    if "til" in tags:
        return SECTION_TIL, f"- [ ] {cleaned}" if cleaned else f"- [ ] {message_link(chat_id, thread_id, message['message_id'])}"

    if "link" in tags or urls:
        url = urls[0] if urls else ""
        caption = URL_RE.sub("", cleaned).strip(" -—")
        caption = re.sub(r"\s+", " ", caption).strip()
        if url and caption:
            return SECTION_BOOKMARKS, f"- [ ] {url} — {caption}"
        if url:
            return SECTION_BOOKMARKS, f"- [ ] {url}"
        # #link tag with no URL in text: fall through to unsorted with cleaned text.

    if cleaned:
        return SECTION_UNSORTED, f"- [ ] {cleaned}"

    # No text/caption at all (e.g. a bare video/photo upload) - we can't
    # archive the media itself, so leave a breadcrumb back to Telegram.
    link = message_link(chat_id, thread_id, message["message_id"])
    return SECTION_UNSORTED, f"- [ ] [attachment with no caption]({link}) — review in Telegram"


def insert_into_section(lines: list[str], heading: str, new_bullets: list[str]) -> list[str]:
    try:
        start = lines.index(heading)
    except ValueError:
        # Section doesn't exist (shouldn't happen with the standard template) - append at end.
        return lines + ["", heading, "", *new_bullets]

    end = len(lines)
    for i in range(start + 1, len(lines)):
        if lines[i].startswith("## "):
            end = i
            break

    section = lines[start:end]
    # Placeholder is a single empty checkbox: "- [ ]" with nothing after it.
    body = [ln for ln in section[1:] if ln.strip()]
    is_placeholder_only = len(body) == 1 and body[0].strip() == "- [ ]"

    if is_placeholder_only:
        # Replace the empty placeholder line with the real entries.
        placeholder_idx = next(i for i in range(start + 1, end) if lines[i].strip() == "- [ ]")
        return lines[:placeholder_idx] + new_bullets + lines[placeholder_idx + 1 : end] + lines[end:]

    # Otherwise append after the last bullet line in the section, before the
    # trailing blank line that precedes the next heading.
    last_content = end
    while last_content > start + 1 and not lines[last_content - 1].strip():
        last_content -= 1
    return lines[:last_content] + new_bullets + lines[last_content:end] + lines[end:]


def main() -> int:
    token = os.environ["TELEGRAM_BOT_TOKEN"]
    chat_id = int(os.environ["TELEGRAM_CHAT_ID"])
    thread_id = int(os.environ["TELEGRAM_INBOX_THREAD_ID"])

    state = load_state()
    offset = state.get("offset", 0)

    updates = api_call(token, "getUpdates", {"offset": offset, "timeout": 0, "allowed_updates": json.dumps(["message"])})

    entries: dict[str, list[str]] = {SECTION_UNSORTED: [], SECTION_TIL: [], SECTION_BOOKMARKS: []}
    max_update_id = offset - 1

    for update in updates:
        max_update_id = max(max_update_id, update["update_id"])
        message = update.get("message")
        if not message:
            continue
        if message.get("chat", {}).get("id") != chat_id:
            continue
        if message.get("message_thread_id") != thread_id:
            continue
        if message.get("from", {}).get("is_bot"):
            continue

        section, bullet = format_entry(message, chat_id, thread_id)
        entries[section].append(bullet)

    new_offset = max_update_id + 1
    state_changed = new_offset != offset
    state["offset"] = new_offset

    total_new = sum(len(v) for v in entries.values())
    if total_new == 0:
        if state_changed:
            save_state(state)
            print("No new INBOX-topic messages; advanced offset only.")
        else:
            print("No new updates.")
        return 0

    text = INBOX_PATH.read_text()
    lines = text.split("\n")
    for heading in (SECTION_UNSORTED, SECTION_TIL, SECTION_BOOKMARKS):
        if entries[heading]:
            lines = insert_into_section(lines, heading, entries[heading])
    INBOX_PATH.write_text("\n".join(lines))

    save_state(state)
    print(f"Added {total_new} entr{'y' if total_new == 1 else 'ies'} to INBOX.md.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
