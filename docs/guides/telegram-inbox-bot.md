# Telegram → INBOX bot

A GitHub Actions workflow polls a Telegram bot for messages posted in one
topic of a private group and appends them to [`INBOX.md`](https://github.com/makualiyev/wiki-repo/blob/main/INBOX.md).
No server to run — it's a scheduled job (`.github/workflows/telegram-inbox.yml`)
that calls the script at `scripts/telegram_inbox.py` and pushes straight to
`main`.

## How it sorts messages

Every message posted in the group's **INBOX** topic gets one bullet in
`INBOX.md`, chosen by hashtag:

| You write in the topic | Lands in |
| --- | --- |
| `#til some fact` | `## To turn into a TIL` |
| a message containing a URL, or tagged `#link` | `## Bookmarks to file` (as `- [ ] <url> — <your caption>`) |
| anything else | `## Unsorted` |

A video/photo uploaded with no caption and no link can't be archived into a
markdown file, so it's added to `## Unsorted` as a link back to the
Telegram message instead — go grab it manually.

The bot only reads messages inside the configured INBOX topic; everything
else in the group (other topics, checklists, etc.) is ignored.

## One-time setup

1. **Create the bot.** Talk to [@BotFather](https://t.me/BotFather), run
   `/newbot`, and save the token it gives you.
2. **Let it read group messages.** Still in BotFather: `/setprivacy` →
   select your bot → **Disable**. Without this the bot only sees messages
   that start with `/commands`.
3. **Add the bot to the group** as a regular member (topics must already be
   enabled on the group, which they are).
4. **Find the group's chat ID.** Add [@RawDataBot](https://t.me/RawDataBot)
   to the group temporarily (or check `getUpdates` after step 3), send any
   message, and read `chat.id` — a negative number like `-1001234567890`.
   Remove the helper bot afterwards.
5. **Find the INBOX topic's thread ID.** In Telegram Desktop or the web
   app, open the **INBOX** topic → **⋮** → **Copy Link**. The link looks
   like `https://t.me/c/1234567890/45` — `45` is the thread ID.
6. **Add repo secrets** under **Settings → Secrets and variables → Actions**:
   - `TELEGRAM_BOT_TOKEN`
   - `TELEGRAM_CHAT_ID` (from step 4)
   - `TELEGRAM_INBOX_THREAD_ID` (from step 5)
7. **Allow the workflow to push.** Under **Settings → Actions → General →
   Workflow permissions**, select **Read and write permissions**.
8. Run the workflow once manually (**Actions → Sync Telegram INBOX topic →
   Run workflow**) to confirm it picks up a test message.

## Schedule

The workflow runs once a day (`cron: "0 7 * * *"`, UTC) and can also be
triggered manually from the Actions tab. Edit the cron expression in
`.github/workflows/telegram-inbox.yml` to change the frequency.

## State

`.github/telegram-state.json` tracks the last Telegram `update_id` the bot
has consumed, so re-runs never reprocess old messages. It's committed
alongside `INBOX.md` changes — don't edit it by hand.
