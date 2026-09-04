# TamaPoke web installer

Local page that flashes firmware 3.13.1 and pushes sprites / art / save files
over Web Serial. Chrome or Edge. Serve from this folder:

```bat
start_installer.cmd
```

or `python -m http.server 8001` and open http://localhost:8001/

## Updating

Leave **Erase device** unchecked. The save lives in NVS on the chip, not on
the microSD. The card also gets a silent copy at `/saves/latest.tkps`.

## Step 3 — save backup

After Connect: **Save on card** shows who is stored, **Download backup** pulls
`tamapoke.tkps` onto this computer, **Restore from card** writes it back,
**Restore a file** uploads a `.tkps` you kept elsewhere.
