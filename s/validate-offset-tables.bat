@echo off

echo %1\startup.vcs
vccnow v %1\startup.vcs
echo %1\effects.vcs
vccnow v %1\effects.vcs
echo %1\magic.vcs
vccnow v %1\magic.vcs

for %%f in (%1\*.map) do (
  echo %1\%%~nf.map
  vccnow v %1\%%~nf.map
)