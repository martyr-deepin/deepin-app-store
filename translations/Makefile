
all: update-qm update-desktop

update-qm:
	lupdate -recursive -I. ../src/app ../src/ui -ts deepin-app-store.ts

update-desktop:
	# This command is provided by `deepin-gettext-tools`
	deepin-desktop-ts-convert desktop2ts ../src/resources/deepin-app-store.desktop desktop
