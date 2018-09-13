# PSVita bare-metal loader

## What's this?

This is a kernel plugin that lets you run bare-metal (i.e. without an OS beneath) payloads in ARMv7 non-secure System mode.

## How does it work?

At first, the plugin allocates a physically contiguous buffer where it loads the payload (`PAYLOAD_PATH` in `config.h`).
Then it triggers a power standby request and when PSVita OS is about to send the [Syscon](https://wiki.henkaku.xyz/vita/Syscon)
command to actually perform the standby, it changes the request type into a soft-reset and the resume routine address to a custom one (`resume.S`).

Once the PSVita wakes from the soft-reset, the custom resume routine identity maps the scratchpad (address [0x1F000000](https://wiki.henkaku.xyz/vita/Physical_Memory)) using a 1MiB section.
Then the payload bootstrap code (`payload_bootstrap.S`) is copied to the scratchpad and a jump is done to that location afterwards (passing some parameters such the payload physical address).

Since the payload bootstrap code is now in an identity-mapped location, it can proceed to disable the MMU and copy the payload from the previously allocated physically contiguous buffer to its destination address
(`PAYLOAD_PADDR` in `config.h`), to finally jump to it.

## Instructions

**Compilation**:

* [vitasdk](https://vitasdk.org/) is needed.

**Installation**:

1. Copy your payload to your PSVita (default path is `ux0:/baremetal/payload.bin`)
2. Copy `baremetal-loader.skprx` to your PSVita
3. Load the plugin

## Example bare-metal payload

Check [vita-baremetal-sample](https://github.com/xerpi/vita-baremetal-sample) as sample payload to be loaded with this plugin.

## Credits

Thanks to everybody who contributes to [wiki.henkaku.xyz](https://wiki.henkaku.xyz/) and helps reverse engineering the PSVita OS.
