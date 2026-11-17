# MaeShare 1.0.0

First public release: a LocalSend-compatible file transfer client for the Nokia N900
(Maemo 5 / Fremantle), including real integration into the system Share menu.

## Highlights

- **LocalSend protocol over LAN** — works with the official LocalSend clients on
  Windows, macOS, Linux, Android and iOS (port 53317, HTTPS, text messages supported).
- **System Share integration** — pick one or more photos or a video in Photos / Image
  Viewer, tap *Share*, choose MaeShare, and the app opens on its own device picker with
  the media already selected. Implemented through Maemo's native `libsharing` plugin
  mechanism (service XML + `libmaeshare.so`), not through `.desktop` MIME handlers.
- **Receive / Send / Text / History / Settings** — one-tap receive radar, quick save,
  receive history with *Open folder* / *Clear history*, multi-file sending (20 files),
  text messages delivered as LocalSend text bubbles.
- **Manual address entry** — the bullseye icon on the send tab opens an address page for
  peers that multicast never reaches (blocked or filtered traffic, another subnet): the
  app probes the typed IPv4, pins the peer's certificate fingerprint and remembers it
  across restarts. Invalid or non-local addresses are rejected before connecting.
- **Four UI languages** — English, Simplified Chinese, Traditional Chinese (HK/TW), each
  following the system locale.
- **Keep awake during transfers** via MCE `req_display_blanking_pause`; normal screen
  timeout returns afterwards. The app never rewrites lock-screen settings.
- **Share staging cleans itself up** — copies made for a system share go to the hidden
  `~/.maeshare-share/`, are deleted when the send finishes or is cancelled, and anything
  orphaned is swept after 7 days at startup.

## Install

```sh
# on the device, as root
dpkg -i maeshare_1.0.0_armel.deb
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
