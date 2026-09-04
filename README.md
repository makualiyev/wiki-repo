# wiki-repo

A personal knowledge base — guides, cheatsheets, *Today I Learned* notes, and
categorized bookmarks. Built as a searchable static site with
[MkDocs Material](https://squidfunk.github.io/mkdocs-material/).

## Structure

```
docs/
  index.md            # site landing page / table of contents
  guides/             # long-form, opinionated write-ups
  cheatsheets/        # "how do I do X again" quick references
  til/                # Today I Learned — small dated single-fact notes
  bookmarks/          # categorized links
  assets/             # images used by the docs
templates/            # copy-paste starting points for new notes
INBOX.md              # frictionless capture; triage into docs/ later
mkdocs.yml            # site configuration
```

## Preview locally

```bash
pip install -r requirements.txt
mkdocs serve          # live-reloading preview at http://127.0.0.1:8000
```

## Publish

Pushing to `main` builds and deploys the site to GitHub Pages via
[`.github/workflows/deploy.yml`](.github/workflows/deploy.yml). To turn it on:
**Settings → Pages → Build and deployment → Source: GitHub Actions**. After the
first deploy, set `site_url` in `mkdocs.yml` to the published URL.

## Adding a note

1. Copy the matching file from `templates/` into the right folder under `docs/`.
2. Add it to `nav:` in `mkdocs.yml`.
3. `mkdocs serve` to preview, then commit.

## Telegram → INBOX bot

A scheduled GitHub Action ([`telegram-inbox.yml`](.github/workflows/telegram-inbox.yml))
pulls messages from one topic of a private Telegram group straight into
`INBOX.md`. See [`docs/guides/telegram-inbox-bot.md`](docs/guides/telegram-inbox-bot.md)
for setup and how messages get sorted.
