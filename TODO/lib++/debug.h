// Original source from lib3d, adapted for my purposes.
//
// juergen.sawinski@urz.uni-heidelberg.de

// Copyright (C) 1996 Keith Whitwell.
// This file may only be copied under the terms of the GNU Library General
// Public License - see the file COPYING in the lib3d distribution.

#ifndef __DEBUG_H_
#define __DEBUG_H_

#include <iostream.h>
#include <strstream.h>


// Objects of this class have debugging facilities which may be turned
// on or off per-class or more specifically if objects have individual
// names, as returned by getName().

extern ostream &endlog(ostream& outs);

class Debug
{
friend inline ostream &debug();
friend inline ostream &err();
friend ostream &endlog(ostream& outs);
friend bool debugging() { return !Debug::noDebug; }

public:
    Debug();
    virtual ~Debug() {}

    virtual const char *getName() const { return ""; }
    bool isGood() const { return good; }
    bool isBad() const { return !good; }
    static void setDebugConfigPath( const char * );
    
    friend ostream& operator<<( ostream &s, const Debug &d ) {
	return d.print( s );
    }

protected:
    void setGood( bool p = true ) { good = good && p; }
    void setBad() { good = false; }
    virtual void reset() { good = true; }

    virtual ostream &print( ostream & ) const;

    ostream &debug();
    ostream &debug() const;
    bool debugging();

private:
    void initDebug();
    static void reconfigure();

private:
    bool good;
    int init;
    ostream *_debug;

    enum { bufferSize = 1024 };

    static int initLevel;
    static bool noDebug;
    static char **tok;
    static int nrTok;
    static int activeTok;
    static char configPath[256];
    static ostrstream errStr;
    static ostrstream debugOn;
    static ostrstream debugOff;
    static char errBuf[];
    static char debugOnBuf[];
    static char debugOffBuf[];
};

// If you have debugging information which would be printed so often
// as to affect performance, a reasonable structure is:
//
//    if_debug {
//        debug() << ... complex business ... << endlog;
//    }

#ifndef DEBUG
#   define if_debug if(0)
#else 
#   define if_debug if(debugging())
#endif


inline ostream &
Debug::debug() 
{
#ifndef DEBUG
    return Debug::debugOff;
#else
    if (init != initLevel) initDebug();
    return *_debug << "Class " << getName() << ": ";
#endif
}

inline ostream &
Debug::debug() const
{
#ifndef DEBUG
    return Debug::debugOff;
#else
    if (*_debug == debugOn) {
	return debugOn << "Class " <<  getName() << ": ";
    } else {
	return debugOff;
    }
#endif
}

inline ostream &
debug()
{
#ifndef DEBUG
    return Debug::debugOff;
#else
    ostrstream *xx;

    if (Debug::noDebug) { 
	xx = &Debug::debugOff;
    } else {
	xx = &Debug::debugOn;
    }

    return *(ostream *)xx;
#endif
}

inline ostream &
err()
{
    return Debug::errStr;
}

inline bool
Debug::debugging()
{ 
    if (init != initLevel) initDebug();
    return _debug == &debugOn;
}



#endif






