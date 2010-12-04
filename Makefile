all:
	xcodebuild -alltargets -configuration Release
	xcodebuild -alltargets -configuration Debug

run:
	cp -R build/Debug/CtagSpotlight.mdimporter /Library/Spotlight/
	mdimport -L 2> /dev/null
	@mdimport -d2 main.c
	rm -Rf /Library/Spotlight/CtagSpotlight.mdimporter
	mdimport -L 2> /dev/null

install:
	cp -R build/Debug/CtagSpotlight.mdimporter /Library/Spotlight/
	mdimport -L 2> /dev/null

uninstall:
	rm -Rf /Library/Spotlight/CtagSpotlight.mdimporter
	mdimport -L 2> /dev/null

clean:
	xcodebuild clean -alltargets -configuration Debug
	xcodebuild clean -alltargets -configuration Release

installed:
	mdimport -L 2>&1 | grep Ctag

validateschema:
	mdcheckschema schema.xml

