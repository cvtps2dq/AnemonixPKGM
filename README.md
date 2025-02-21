# Anemonix - Elemental Package Manager
![anemonix](https://github.com/user-attachments/assets/c4724a33-9c89-4b28-96c5-7a5408dddfde)

### Anemonix is a lightning-fast, package manager designed for modern Linux systems. Like the wind, it brings change—delivering fresh packages with transparency, speed, and flexibility.

## 🌪 Why Anemonix?

### Unlike legacy package managers tied to dpkg, rpm, or libalpm, Anemonix redefines package management with:

✅ Source Builds – Compile from source for maximum customization.

✅ Binary Cache – Install prebuilt packages when speed matters.


✅ Deterministic & Isolated Builds – Ensure reproducible results with sandboxed environments.

✅ Minimal & Modern – No legacy baggage, just raw power.

## 📦 Package Format (.apkg)

### Anemonix packages are simple, transparent, and powerful:

```
package-name/
├── anemonix.yaml    # Package metadata
├── build.anemonix   # Build script (Bash/Python)
├── patches/         # Optional patches
└── binaries/        # Prebuilt binaries (optional)
```

### Example anemonix.yaml

```yaml
name: nginx
version: 1.25.3
license: BSD-3-Clause
deps:
  - libssl >=3.0.0
  - zlib
build_deps:
  - cmake
  - gcc
arch: [x86_64, aarch64]
```

## ⚙ How Anemonix Works

```bash
# Install from source (preferred)
anemo install --build nginx

# Install prebuilt binary (if available)
anemo install nginx

# Remove package
anemo remove nginx

# Update repository index
anemo sync

# Search for a package
anemo search "python.*"

# Build a package from local source
anemo build ./nginx.apkg

# Check installed packages for integrity
anemo audit

```

## 🔗 Repository Structure

Anemonix uses simple HTTPS repositories:

```
https://repo.anemonix.org/
├── index.db      # Signed package list (SQLite)
└── packages/
    ├── nginx-1.25.3.apkg
    └── libssl-3.0.8.apkg
```

### Security & Integrity

✅ Ed25519 Signatures – Verify all packages. 

✅ SHA-256 Checksums – Prevent tampering.

✅ Containerized Builds – Use namespaces/cgroups for security.

✅ Reproducible Builds – Verify binaries by rebuilding from source.

##🏗 How Packages Are Built

### Source-Based Workflow

1️⃣ Fetch & verify source tarball.

2️⃣ Apply patches (if any).

3️⃣ Run build.anemonix in an isolated container.

4️⃣ Install artifacts to a sandbox directory.

5️⃣ Generate a .apkg for future reuse.

### Binary Workflow

1️⃣ Verify Ed25519 signature.

2️⃣ Check binary compatibility (architecture, libc).

3️⃣ Install instantly if valid.


## 🚧 Challenges & Future Roadmap

### 🕐 Build Time
•	Large packages like GCC take time.
 
✔ Solution: Community-maintained binary mirrors.

### 🌍 Cross-Platform Support
•	Need ARM, RISC-V, and x86_64 support.
 
✔ Solution: Integrate QEMU for cross-compilation.

## 🚀 Get Started Today!

1️⃣ Clone the repository:

```
git clone https://github.com/yourrepo/anemonix.git
cd anemonix
```

2️⃣ Install dependencies.
(libsoduim, libssl, yaml-cpp, sqlite3)

3️⃣ Build and test your first p
📜 License: GPL | 🔥 Contribute: PRs welcome! | 📣 Community: Join the discussion!
