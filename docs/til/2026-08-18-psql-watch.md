# psql `\watch` for live query monitoring

*2026-08-18 · tags: postgresql, cli*

`psql` can re-run the previous query on an interval with `\watch`, turning any
query into a live dashboard — no external tooling needed.

```sql
SELECT state, count(*)
FROM pg_stat_activity
GROUP BY state;
\watch 2   -- re-run every 2 seconds
```

Stop it with <kbd>Ctrl</kbd>+<kbd>C</kbd>. Handy for watching connection counts,
replication lag, or a long-running job's progress.

!!! note "This is a sample TIL"
    It's here to show the shape of a note and prove the section renders. Delete
    it once you've written your own.
