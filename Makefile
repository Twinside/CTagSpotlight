all:
	xcodebuild -alltargets -configuration Debug -configuration Release

run:
	cp -R build/Debug/CtagSpotlight.mdimporter /Library/Spotlight/
	mdimport -L 2> /dev/null
	@mdimport -d2 main.c
	rm -Rf /Library/Spotlight/CtagSpotlight.mdimporter
	mdimport -L 2> /dev/null

install:
	cp -R build/Debug/CtagSpotlight.mdimporter /Library/Spotlight/
	mdimport -L 2> /dev/null

clean:
	rm -Rf /Library/Spotlight/CtagSpotlight.mdimporter
	mdimport -L 2> /dev/null

installed:
	mdimport -L 2>&1 | grep Ctag

