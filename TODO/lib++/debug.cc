// Copyright (C) 1996 Keith Whitwell.
// This file may only be copied under the terms of the GNU Library General
// Public License - see the file COPYING in the lib3d distribution.

#include "Debug.h"

#include <iostream.h>
#include <fstream.h>
#include <strstream.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <std.h>

int Debug::initLevel = 0;
int Debug::nrTok = 0;
int Debug::activeTok = 0;
char **Debug::tok = 0;
char Debug::configPath[256];

char Debug::errBuf[bufferSize];
char Debug::debugOnBuf[bufferSize];
char Debug::debugOffBuf[10];

ostrstream Debug::errStr(errBuf, sizeof(errBuf));
ostrstream Debug::debugOn(debugOnBuf, sizeof(debugOnBuf));
ostrstream Debug::debugOff(debugOffBuf, sizeof(debugOffBuf));

#ifndef DEBUG
bool Debug::noDebug = true;
#else
bool Debug::noDebug = false;
#endif


Debug::Debug()
    : good(true),
      init(-1),
      _debug(&debugOff)
{
}

void
Debug::initDebug()
{
    _debug = &debugOff;

    if (!*getName()) return;
    if (noDebug) return;

    if (initLevel == 0) {
	setDebugConfigPath(".");
    }

    const char *name = getName();

    for (int i = 0; i < activeTok && _debug == &debugOff ; i++) {
	if (strstr(name, tok[i]) != 0) {
	    _debug = &debugOn;
	} 
    }

    init = initLevel;
}

void
Debug::setDebugConfigPath( const char *path )
{
    ostrstream p( configPath, sizeof(configPath) );
    
    p << path << "/debug.conf" << ends;
    if (!p.good()) {
	err() << "Config path too long" << endlog;
    }	

    reconfigure();		// Increments initLevel.
}

void
Debug::reconfigure()
{
    char s[256];
    int lines = 0;
    ifstream in( configPath );
    static int notified = false;

    if (!in) {
	if (!notified) {
	    ::debug() << "No debug.conf file present - debugging disabled" 
		      << endlog;
	    notified = true;
	}
	return;
    }

    while (in.getline(s, sizeof(s))) {
	lines++;
    }
    
    while( nrTok ) {
	free( tok[--nrTok] );
    }
    delete [] tok;
    tok = new char *[lines];

    // in.seekg(0);
    ifstream in2( configPath );
    
    for(activeTok = 0; in2.getline(s, sizeof(s)) && activeTok < lines ;) {
	tok[activeTok] = new char[strlen(s)+1];
	strcpy(tok[activeTok++], s);
    }
    initLevel++;
}



ostream &
endlog( ostream &out ) 
{
    out << endl;

    if (*Debug::errBuf) {
	Debug::errStr << ends;
	cout << Debug::errBuf << flush;
	Debug::errBuf[0] = 0;
	Debug::errStr.seekp(0);
    } 

    if (*Debug::debugOnBuf) {
	Debug::debugOn << ends;
	cout << Debug::debugOnBuf << flush;
	Debug::debugOnBuf[0] = 0;
	Debug::debugOn.seekp(0);
    } 

    return out;
}

ostream &
Debug::print( ostream &out ) const
{
    return out << "Class " <<  getName() 
               << "(" << (const void *)this << ")"
	       << " status: " << (good ? "good" : "bad");
}




class Test : public Debug
{
public:
    Test() {}
    ~Test() {}

    void test() {
	debug() << "hello, world" << endlog;
    }

protected:
    const char *getName() const { return "Test"; }
};
    

main()
{
    Test test;

    test.test();
}   
