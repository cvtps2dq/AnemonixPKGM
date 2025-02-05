#!/usr/bin/sh

tar -czvf test-package.apkg test-package
tar -czvf dependency-a.apkg dependency-a
tar -czvf dependency-b.apkg dependency-b
tar -czvf test-package-with-dependency.apkg test-package-with-dependency