# Setup

In the working directory, you will find a tarball containing Meson.  
You need to extract it and use the unpacked version directly.

Ensure the following tools are installed on your system:

- `clang`
- `clang-tools`
- `gcc` / `g++`
- `git`
- `make` or `ninja`
- `tar`
- `meson`

---

# Additional APT Packages

Install these for development and debugging support:

- `build-essential`
- `gdb`
- `valgrind`
- `lsof`
- `strace`
- `curl`, `wget`
- `zip`, `unzip`
- `tree`
- `htop`
- `python3`
- `ffmpeg` suite
- `graphviz`

---

# Network

Due to OpenAI restrictions, network connectivity inside the container is **limited and unstable**.  
Some network operations may silently fail or hang — this is expected. Don't be alarmed.

---

# Instructions

Follow the user prompts carefully and adhere to the coding conventions outlined in the `README`.

---

## Comments

Avoid excessive commenting.  
Only add comments where they are genuinely helpful or clarify non-obvious logic.  
Use [Doxygen](https://www.doxygen.nl/manual/docblocks.html)-style comments where appropriate to support documentation generation.

