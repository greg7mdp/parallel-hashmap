- update version in phmap_config.h
- update version in comment on top of CMakeLists.txt
- git commit
- git push
- create the new release on github (tag `v1.3.8` - use semantic versioning)
- download the tar.gz from github, and use `sha256sum parallel-hashmap-1.3.8.tar.gz` on linux to get the sha256

## conan

- fork and clone [conan-center repo](https://github.com/conan-io/conan-center-index)
     (or sync +  git pull)
- git checkout master
- git checkout -b phmap_1.3.8
- update: `recipes/parallel-hashmap/all/conandata.yml` and `recipes/parallel-hashmap/config.yml`
- sudo pip install conan -U 
- cd recipes/parallel-hashmap/all
- conan create conanfile.py parallel-hashmap/1.3.8@ -pr:b=default -pr:h=default 
- git diff
- git commit -am "[parallel-hashmap] Bump version to 1.3.8"
- git push origin phmap_1.3.8 
- create PR like [this](https://github.com/conan-io/conan-center-index/pull/13161)


## vcpkg

- fork and clone [vcpkg repo](https://github.com/microsoft/vcpkg)
     (or sync +  git pull)
- git checkout -b phmap_1.3.8
- update ports/parallel-hashmap/portfile.cmake and ports/parallel-hashmap/vcpkg.json

in windows, non-cygwin console

- set VCPKG_ROOT=
- vcpkg install parallel-hashmap --triplet x64-windows
- # update sha in portfile.cmake - run `sha512sum parallel-hashmap-1.3.8.tar.gz` on linux
- git diff
- git commit -am "[parallel-hashmap] Bump version to 1.3.8"
- vcpkg x-add-version --all --overwrite-version ## (or ./vcpkg.exe --no-dry-run upgrade )
- git diff -am "[parallel-hashmap] run x-add-version"
- git commit ...
- git push origin phmap_1.3.8 
