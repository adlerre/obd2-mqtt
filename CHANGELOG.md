# [0.11.0](https://github.com/adlerre/obd2-mqtt/compare/v0.10.0...v0.11.0) (2024-12-13)


### Bug Fixes

* add link to profiles folder ([8a15776](https://github.com/adlerre/obd2-mqtt/commit/8a157764e692410db4d64d8be08ab12b0164959e))
* add missing description how to use single bytes from var's ([e38c52c](https://github.com/adlerre/obd2-mqtt/commit/e38c52c0dd6f7fffe97d68effcf1461c08d7aa70))
* strip also empty objects ([96f8adc](https://github.com/adlerre/obd2-mqtt/commit/96f8adc0de2cdd9304d29316dbc9ae9826f79dde))
* use only enabled var ([7c5c89d](https://github.com/adlerre/obd2-mqtt/commit/7c5c89d558a169b82204239eea2c902ab2178835))


### Features

* add support for TLS/SSL connections to MQTT/WS ([#37](https://github.com/adlerre/obd2-mqtt/issues/37)) ([278ec15](https://github.com/adlerre/obd2-mqtt/commit/278ec158b92d3b6c97ef745009a8621264bb0349))
* use (nice) toast instead of alert ([#36](https://github.com/adlerre/obd2-mqtt/issues/36)) ([2405372](https://github.com/adlerre/obd2-mqtt/commit/24053722537ad4dfba43ce4054aab404686184a3))

# [0.10.0](https://github.com/adlerre/obd2-mqtt/compare/v0.9.1...v0.10.0) (2024-12-11)


### Bug Fixes

* add definitions to index.ts ([b41d94a](https://github.com/adlerre/obd2-mqtt/commit/b41d94a4f45f6117aec53a505168366e5c2ce7f9))
* add missing hint ([84ced62](https://github.com/adlerre/obd2-mqtt/commit/84ced627eb26438e9236b19bf746094ebb40fb3f))
* add missing options to set measurement/diagnostic ([15746ae](https://github.com/adlerre/obd2-mqtt/commit/15746aea0efd95c7a8e86535bc997c5858f93476))
* add pattern with allowed chars to name ([ea9e91d](https://github.com/adlerre/obd2-mqtt/commit/ea9e91d4b0d7e68e9e425b4a795c974ac0f2d85b))
* clear old states before save ([493acb6](https://github.com/adlerre/obd2-mqtt/commit/493acb6e65ebd0e59d244951b4f0c5ba4aede161))
* describe some more options ([280a964](https://github.com/adlerre/obd2-mqtt/commit/280a9647c3242db5eac678ac7310eeb20916ecf2))
* fix code issues ([198c2d4](https://github.com/adlerre/obd2-mqtt/commit/198c2d437fee659043b9ea47c24d38a5868d217f))
* fix crash on AP connect ([629540f](https://github.com/adlerre/obd2-mqtt/commit/629540f7428f4d0d89c01bfab37f575e457c89ba))
* fix fuelRate state ([65befdb](https://github.com/adlerre/obd2-mqtt/commit/65befdb4a3b8e33fe80f6b114e69436ca736eb0f))
* improve expression description ([6527840](https://github.com/adlerre/obd2-mqtt/commit/652784053ab07957f87988d1f82a78483d65ed4e))
* isn't required to bind class ([66ad0b6](https://github.com/adlerre/obd2-mqtt/commit/66ad0b65331691759d98c63b21b28d7fe1ffa6d4))
* name description ([ad669a4](https://github.com/adlerre/obd2-mqtt/commit/ad669a4e3a40bb63720331ba823c38143809d0f0))
* oom on sim800l ([052323b](https://github.com/adlerre/obd2-mqtt/commit/052323bc1dd26778b894bed29af3014bb5811b99))
* remove all .txt* to save some space ([5dd08c1](https://github.com/adlerre/obd2-mqtt/commit/5dd08c183f8f2036a2f1a1f7b4474799ef90a9e3))
* remove unneeded variable ([460e28e](https://github.com/adlerre/obd2-mqtt/commit/460e28e99394e4b4c4aa2cace8963bba223905f2))
* rename strip method and use also on generate download ([8098248](https://github.com/adlerre/obd2-mqtt/commit/80982481d484d80f145ecb653a05243bcdbafacc))
* rewrite json parsing ([544e140](https://github.com/adlerre/obd2-mqtt/commit/544e14012183d40ca684991822b8d2fc3ef94e7c))
* set measurement to true ([b6a9376](https://github.com/adlerre/obd2-mqtt/commit/b6a93761c5d3976ac647ead441d7a13a153ed7f3))
* use strlcpy to prevent buffer overflow ([d108c77](https://github.com/adlerre/obd2-mqtt/commit/d108c772ef15e3f3e47626c0d30bf0bfbb684943))


### Features

* add api methods to read/write obd states ([c3dc2fc](https://github.com/adlerre/obd2-mqtt/commit/c3dc2fc606b7460fcbb5e8c0c09540f500fa60f5))
* add profile for VW e-up! ([79c21a0](https://github.com/adlerre/obd2-mqtt/commit/79c21a0780a05857f820d0c835845880c0ee964f))
* add profile with states in imperial format ([4f95adb](https://github.com/adlerre/obd2-mqtt/commit/4f95adbc296dbe06ed7e2f4477e4fbc24a42429c))
* implement GUI to edit OBD states ([a08d260](https://github.com/adlerre/obd2-mqtt/commit/a08d26039016471a1a3f071375fe70100e0b182a))
* implement load of obd states from json ([37e74bb](https://github.com/adlerre/obd2-mqtt/commit/37e74bb5d673adf7929b31d1eb7279987a5c41b7))
* implement save of obd states to json ([6fe8dd7](https://github.com/adlerre/obd2-mqtt/commit/6fe8dd77ca82070474a0f2188ce8b673a7023498))
* math expression parser, with var resolver and more ([867783b](https://github.com/adlerre/obd2-mqtt/commit/867783ba0561de81da0d566fcf0ecffe5eef9b8b))
* remove measurement system support, build-in functions can be used ([8c49edd](https://github.com/adlerre/obd2-mqtt/commit/8c49edd16eca86e187dc5dccc951bff31635489d))
* rework obd states to make it customizable ([022d620](https://github.com/adlerre/obd2-mqtt/commit/022d6208c84b19db559ea1e8e13f3f3414abb609))
* save also states after ota update ([b0f9ca1](https://github.com/adlerre/obd2-mqtt/commit/b0f9ca18eba0782413d0f750d0a13a2e6f971904))
* update README with some explanations ([ea3698d](https://github.com/adlerre/obd2-mqtt/commit/ea3698d538ce60831bd995c05648e9f3053eeb46))

## [0.9.1](https://github.com/adlerre/obd2-mqtt/compare/v0.9.0...v0.9.1) (2024-11-22)


### Bug Fixes

* send only changed data to save traffic ([d6f69f7](https://github.com/adlerre/obd2-mqtt/commit/d6f69f702f4bc7f79a2a176fbd2f37d75dce81c7))
* use shorter interval ([d850248](https://github.com/adlerre/obd2-mqtt/commit/d8502489ddfecc1c9a75874379b54aef61f29745))

# [0.9.0](https://github.com/adlerre/obd2-mqtt/compare/v0.8.0...v0.9.0) (2024-11-21)


### Bug Fixes

* possible buffer overflow ([983f88d](https://github.com/adlerre/obd2-mqtt/commit/983f88de0b38d77b07d3d81cf0cfbbaa111b8c38))
* remove ToDos ([5d6dc45](https://github.com/adlerre/obd2-mqtt/commit/5d6dc458008dccbabecbd785a25cf168c6b7cd71))
* remove unused imports ([59674e7](https://github.com/adlerre/obd2-mqtt/commit/59674e70ae455bd2e4fb361ff27084b473376aef))


### Features

* add more states, odometer, number of DTCs and print theas to console ([a3cf1a7](https://github.com/adlerre/obd2-mqtt/commit/a3cf1a71ee97560ffe72ee558611222b82700fcb))
* use custom implementation to check if pid is supported ([6740272](https://github.com/adlerre/obd2-mqtt/commit/6740272180d33000296dd41153d59e139521886c))

# [0.8.0](https://github.com/adlerre/obd2-mqtt/compare/v0.7.3...v0.8.0) (2024-11-20)


### Features

* generic OBD states ([#30](https://github.com/adlerre/obd2-mqtt/issues/30)) ([e3e4f17](https://github.com/adlerre/obd2-mqtt/commit/e3e4f1775e91ad8922c3081ef3fe1088cfdc4eae))

## [0.7.3](https://github.com/adlerre/obd2-mqtt/compare/v0.7.2...v0.7.3) (2024-11-20)


### Bug Fixes

* fix issue with measurement system switching ([#29](https://github.com/adlerre/obd2-mqtt/issues/29)) ([b0a424d](https://github.com/adlerre/obd2-mqtt/commit/b0a424df610f62751ba8201cb2508f20a2a4c8c1))

## [0.7.2](https://github.com/adlerre/obd2-mqtt/compare/v0.7.1...v0.7.2) (2024-11-03)


### Bug Fixes

* change ui colors ([1690f31](https://github.com/adlerre/obd2-mqtt/commit/1690f318ef900c8c3d80fc259c41362f7f141c5e))
* improve build of classes and variables ([0a687f9](https://github.com/adlerre/obd2-mqtt/commit/0a687f923706cac9123530f993afd80cfbcd0d0e))
* improve code quality ([33827e4](https://github.com/adlerre/obd2-mqtt/commit/33827e45c38c950fc5ec645198cfbd8551f6fafe))

## [0.7.1](https://github.com/adlerre/obd2-mqtt/compare/v0.7.0...v0.7.1) (2024-11-01)


### Bug Fixes

* add new screenshots ([81675bb](https://github.com/adlerre/obd2-mqtt/commit/81675bb0bea32c0fef1609f130c14f7a65f97226))
* allow to select multiple files ([a28b7a4](https://github.com/adlerre/obd2-mqtt/commit/a28b7a4c3021b699409868faf51dedf5e82fe56c))
* move to http class ([ba2d914](https://github.com/adlerre/obd2-mqtt/commit/ba2d9149e0985d21565caa7127da63295dbd3039))
* reboot console error ([4dd823f](https://github.com/adlerre/obd2-mqtt/commit/4dd823f917292041722aa93cd05834822c89a0ed))

# [0.7.0](https://github.com/adlerre/obd2-mqtt/compare/v0.6.0...v0.7.0) (2024-11-01)


### Features

* own OTA updater implemented ([#24](https://github.com/adlerre/obd2-mqtt/issues/24)) ([003c810](https://github.com/adlerre/obd2-mqtt/commit/003c8109afc5eb4c747faf9f7dcd24f6e6615fec))

# [0.6.0](https://github.com/adlerre/obd2-mqtt/compare/v0.5.1...v0.6.0) (2024-10-28)


### Features

* allow to change network mode for supported devices ([#23](https://github.com/adlerre/obd2-mqtt/issues/23)) ([c8c1f05](https://github.com/adlerre/obd2-mqtt/commit/c8c1f05b6e07777394bfe6825431ac9862001093))

## [0.5.1](https://github.com/adlerre/obd2-mqtt/compare/v0.5.0...v0.5.1) (2024-10-26)


### Bug Fixes

* fix connection issue ([#22](https://github.com/adlerre/obd2-mqtt/issues/22)) ([b4ef71d](https://github.com/adlerre/obd2-mqtt/commit/b4ef71d59bdc4c9be7735feba3cbce7425c07fd0))

# [0.5.0](https://github.com/adlerre/obd2-mqtt/compare/v0.4.1...v0.5.0) (2024-10-23)


### Features

* connect to mqtt over websocket ([#19](https://github.com/adlerre/obd2-mqtt/issues/19)) ([a3562b7](https://github.com/adlerre/obd2-mqtt/commit/a3562b74679ca961e40e676119cfda069635cecd))

## [0.4.1](https://github.com/adlerre/obd2-mqtt/compare/v0.4.0...v0.4.1) (2024-10-22)


### Bug Fixes

* remove dirty flag and show errors direct ([#18](https://github.com/adlerre/obd2-mqtt/issues/18)) ([2961875](https://github.com/adlerre/obd2-mqtt/commit/2961875dae20a582abc987af8e0b2ad8309d153c))

# [0.4.0](https://github.com/adlerre/obd2-mqtt/compare/v0.3.0...v0.4.0) (2024-10-20)


### Bug Fixes

* bluetooth reconnect issue with workaround ([61367d7](https://github.com/adlerre/obd2-mqtt/commit/61367d75f9e00c3f87c5b08db4350011865ad578))
* reset only if connection wasn't stopped ([30e83e2](https://github.com/adlerre/obd2-mqtt/commit/30e83e28ee242c71a319674fd8f6f9ab2b276ce4))
* show mph only with imperial system ([b75ef1f](https://github.com/adlerre/obd2-mqtt/commit/b75ef1f50b31c31c9d29a606391cc5007df502f8))


### Features

* implemented imperial system, may the force be with you ([#12](https://github.com/adlerre/obd2-mqtt/issues/12)) ([4f9ecfa](https://github.com/adlerre/obd2-mqtt/commit/4f9ecfa50178f3a02749ea979043dc681b2d5e4f))

# [0.3.0](https://github.com/adlerre/obd2-mqtt/compare/v0.2.1...v0.3.0) (2024-10-18)


### Features

* implemented autocomplete of discovered BT devices ([#11](https://github.com/adlerre/obd2-mqtt/issues/11)) ([faa2e6b](https://github.com/adlerre/obd2-mqtt/commit/faa2e6b8a1bfa3dd569a4636b14dd4a7e39bcd00))

## [0.2.1](https://github.com/adlerre/obd2-mqtt/compare/v0.2.0...v0.2.1) (2024-10-16)


### Bug Fixes

* add hint to insert sim card ([0890990](https://github.com/adlerre/obd2-mqtt/commit/0890990961930222a3afe8b51f6b7b386b2ea714))
* build also ui on test job ([beb3bb7](https://github.com/adlerre/obd2-mqtt/commit/beb3bb764e131825e9fd1497e02c04878952a0c0))
* build also ui on test job for manual release ([1f8a18b](https://github.com/adlerre/obd2-mqtt/commit/1f8a18b0b840539cee33af8e0d3f2ba0df9dff94))
* inject release version ([47b4e9a](https://github.com/adlerre/obd2-mqtt/commit/47b4e9a4afb186949e238b40b429f705284eae73))
* issue with file system without UI ([da85aac](https://github.com/adlerre/obd2-mqtt/commit/da85aacae5afd0b238fefd91951cd7d3b506371b))
* stop connection on end was called ([fad6dbd](https://github.com/adlerre/obd2-mqtt/commit/fad6dbda588cdeca8a65ae8d97e93ee6c896a4ff))

# [0.2.0](https://github.com/adlerre/obd2-mqtt/compare/v0.1.1...v0.2.0) (2024-10-14)


### Bug Fixes

* reconnect counter ([bd17af9](https://github.com/adlerre/obd2-mqtt/commit/bd17af9fdc3b29b98862ca2f97ac74dadc870333))
* remove unneeded import ([4d84d3f](https://github.com/adlerre/obd2-mqtt/commit/4d84d3f15147fd8816842f89d189d63dd8076ffb))


### Features

* allow to set send data intervals ([3230346](https://github.com/adlerre/obd2-mqtt/commit/3230346a309083c751e94aee1c8d397679b97289))
* complete rework of OBD helper ([8627b87](https://github.com/adlerre/obd2-mqtt/commit/8627b87047cc1aa408caa04abf434139b7177eeb))
