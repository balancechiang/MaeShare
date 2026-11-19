# MaeShare 1.0.1

A maintenance release on top of 1.0.0: the Send screen is rebuilt, text input behaves
properly on a real N900, the Share hand-off is fixed for four separate failure cases, and
the package now installs on a stock Qt 4.7.0 phone.

## Changes

- **Send screen rebuilt** — the selected content is one summary bar (file name, `1 item`,
  `Clear` / `Cancel` in the same row); the floating bottom chip is gone, the screen icons
  are no longer wrapped in their own rounded tiles, and the refresh / manual-address icons
  sit right after *Nearby devices* again.
- **Text input and focus** — fields take focus when tapped and when a page opens, so the
  hardware keyboard and on-screen input land in the right place; the name, manual-address
  and text-message pages each have a `Paste` button that pulls Chinese text from the X
  clipboard.
- **Share hand-off fixes** — staged copies keep their extension (original name → MIME →
  magic-byte fallback), the selected files finally show up in the picker, a stale
  `probe --serve` no longer blocks discovery, and sharing from Photos works even when
  MaeShare is not running.
- **Qt dependency relaxed** — `Depends` lowered from `>= 4.7.4` to `>= 4.7.0`, so a stock
  Maemo 5 install no longer stops at `dpkg -i`; see the Qt / CSSU section in `README.md`.

## Install

```sh
# on the device, as root
dpkg -i maeshare_1.0.1_armel.deb
```

The package name is `maeshare`, and it declares `Conflicts` / `Replaces` on the earlier
`localsend` package — installing it removes that older package and its files in one step,
so nothing needs to be uninstalled first.

It installs the app to `/opt/maeshare/`, the share plugin to
`/usr/lib/sharing/plugins/libmaeshare.so`, the service descriptor to
`/usr/share/sharing/services/maeshare.service.xml` and the panel icon to
`/usr/share/icons/hicolor/48x48/apps/sharing_maeshare.png`.

`sharing-manager` is not D-Bus-activated on every device build; if the MaeShare row
does not appear in the Share panel, start it once:

```sh
DISPLAY=:0 DBUS_SESSION_BUS_ADDRESS=<your session bus> setsid /usr/bin/sharing-manager
```

## Notes

- The Share panel accepts `image/jpeg` and `image/png` only (WebP / HEIC cannot be
  displayed by the device gallery anyway).
- iOS LocalSend 2.2 lists the device only when the identity certificate is ECDSA P-256
  (discovery over BoringSSL silently drops RSA identities); identities generated on first
  launch are ECDSA P-256 and announce protocol `2.1`.
- Tested on a Nokia N900 running Maemo 5 with CSSU.
- Build instructions and the exact Maemo ABI findings behind the share integration are
  documented in `README.md` and `BUILD.md`.
