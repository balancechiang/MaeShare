# MaeShare 1.0.2

A compatibility release: it installs on a phone running the stock Qt 4.7.0, and the Qt section
of the README now says what is actually true.

## Changes

- **Stock Qt 4.7.0** — `Depends` no longer pins a Qt version. Maemo's `4.7.0~git20100909-…`
  sorts *below* `4.7.0` in Debian order, so `>= 4.7.0` made `dpkg -i` refuse to configure on a
  stock device; and since `QtQuick` only exists from 4.7.1 on, the package now also carries
  `qml47/` (same QML, `import Qt 4.7`) which the binary picks from `qVersion()`.
- **README corrected** — the Qt / CSSU section of both READMEs was rewritten: the two QML
  namespaces, the `~` version trap, and what `ls /usr/lib/qt4/imports/` actually tells you.
- **Rest unchanged** — the 1.0.1 Send screen, the paste button and the share fixes are as they were.

## Install

```sh
# on the device, as root
dpkg -i maeshare_1.0.2_armel.deb
```

The package name is `maeshare`, and it declares `Conflicts` / `Replaces` on the earlier
`localsend` package, so nothing needs to be uninstalled first.

It installs the app to `/opt/maeshare/`, the share plugin to
`/usr/lib/sharing/plugins/libmaeshare.so`, the service descriptor to
`/usr/share/sharing/services/maeshare.service.xml` and the panel icon to
`/usr/share/icons/hicolor/48x48/apps/sharing_maeshare.png`.

## Notes

- The `qml47` tree is the stock-Qt path. It has not been tested on a real PR1.3 device, only
  built next to the CSSU one — reports welcome.
- The Share panel accepts `image/jpeg` and `image/png` only; iOS LocalSend 2.2 needs an
  ECDSA P-256 identity (the one generated on first launch already is).
