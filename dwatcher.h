/******************************************************************************
 * File:    dwatcher.h
 *          This file is part of QLA Processing Framework
 *
 * Domain:  QPF.libQPF.DirWatcher
 *
 * Version:  2.0
 *
 * Date:    2016/06/01
 *
 * Author:   J C Gonzalez
 *
 * Copyright (C) 2015-2018 Euclid SOC Team @ ESAC
 *_____________________________________________________________________________
 *
 * Topic: General Information
 *
 * Purpose:
 *   Declare DirWatcher class
 *
 * Created by:
 *   J C Gonzalez
 *
 * Status:
 *   Prototype
 *
 * Dependencies:
 *   Component
 *
 * Files read / modified:
 *   none
 *
 * History:
 *   See <Changelog>
 *
 * About: License Conditions
 *   See <License>
 *
 ******************************************************************************/

#ifndef DIRWATCHER_H
#define DIRWATCHER_H

//============================================================
// Group: External Dependencies
//============================================================

//------------------------------------------------------------
// Topic: System headers
//  - string
//  - vector
//------------------------------------------------------------
#include <string>
#include <map>
#include <queue>
#include <cstdint>

//------------------------------------------------------------
// Topic: External packages
//  none
//------------------------------------------------------------

//------------------------------------------------------------
// Topic: Project headers
//  none
//------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////
// Namespace: QPF
// -----------------------
//
// Library namespace
////////////////////////////////////////////////////////////////////////////
//namespace QPF {

//==========================================================================
// Class: DirWatcher
//==========================================================================
class DirWatcher {

public:
    DirWatcher(std::string pth = std::string());
    ~DirWatcher();

    struct DirWatchEvent {
        std::string      name;
        std::string      path;
        std::uint32_t    mask;
        std::int64_t     size;
        bool             isDir;
    };

    void watch(std::string pth);
    bool nextEvent(DirWatchEvent & event);
    void stop();

public:
    void lockDir(std::string d);
    void unlockDir(std::string d);
    void skip(std::string fname, bool delfile);

private:
    void start();
    bool convertSymbolicLinks(std::string & path, std::string & name);

private:
    int                         fd;
    std::map<int, std::string>  watchedDirs;
    std::queue<DirWatchEvent>   events;
    bool                        keepWatching;
    std::string                 fileToSkip;
    bool                        deleteFileToSkip;
};

//}

#endif  /* DIRWATCHER_H */
