- update version in `phmap_config.h`
- update version in `CITATION.cff`
- update version in comment on top of `CMakeLists.txt`
- update all versions # in `doc/new_release.md`.
- git commit
- git push
- create the new release on github (tag `v2.0.0` - use semantic versioning)
- download the tar.gz from github, and use `sha256sum parallel-hashmap-2.0.0.tar.gz` on linux to get the sha256

## conan

- use [forked repo](https://github.com/greg7mdp/conan-center-index)
- sync fork in github
- git checkout conan-io:master
- git checkout -b phmap_2.0.0
- update: `recipes/parallel-hashmap/all/conandata.yml` and `recipes/parallel-hashmap/config.yml`
- sudo pip install conan -U 
- cd recipes/parallel-hashmap/all
- *does not work* conan create conanfile.py parallel-hashmap/2.0.0@ -pr:b=default -pr:h=default 
  *no version??* update version in `recipes/parallel-hashmap/all/conanfile.py`
- git diff
- git commit -am "[parallel-hashmap] Bump version to 2.0.0"
- git push origin phmap_2.0.0 
- create PR like [this](https://github.com/conan-io/conan-center-index/pull/13161)


## vcpkg

- use [forked repo](https://github.com/greg7mdp/vcpkg)
- sync fork in github
- `git checkout -b phmap_2.0.0`
- update ports/parallel-hashmap/portfile.cmake (the sha512)  and ports/parallel-hashmap/vcpkg.json (the version #)
- commit
- `./bootstrap-vcpkg.sh`
- `vcpkg x-add-version --all --overwrite-version` ## (or `./vcpkg.exe --no-dry-run upgrade` )
- commit
- push
- create PR
