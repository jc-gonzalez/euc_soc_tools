/******************************************************************************
 * File:    dirwatcher.cpp
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
 *   Implement DirWatcher class
 *
 * Created by:
 *   J C Gonzalez
 *
 * Status:
 *   Prototype
 *
 * Dependencies:
 *   none
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

#include "dwatcher.h"

#include <iostream>
#include <thread>

#include <cerrno>
#include <poll.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

//#include "dbg.h"

#include "filetools.h"
using namespace FileTools;

//----------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------
DirWatcher::DirWatcher(std::string pth)
{
    // Initialize file descriptor for inotify API
    fd = inotify_init();
    if (fd == -1) {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    // Add watched directory to table
    if (! pth.empty()) { watch(pth); }

    // Start watch
    keepWatching = true;
    std::thread watcher(&DirWatcher::start, this);
    watcher.detach();
}

//----------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------
DirWatcher::~DirWatcher()
{
}

//----------------------------------------------------------------------
// Method: watch(pth)
//----------------------------------------------------------------------
void DirWatcher::watch(std::string pth)
{
    int wd = inotify_add_watch(fd, pth.c_str(),
                               IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE);
    if (wd == -1) {
//         std::cerr << "Cannot watch '" << pth << "'\n";
        perror("inotify_add_watch");
        exit(EXIT_FAILURE);
    }
    watchedDirs[wd] = pth;
}

//----------------------------------------------------------------------
// Method: convertSymbolicLinks
//----------------------------------------------------------------------
bool DirWatcher::convertSymbolicLinks(std::string & path, std::string & name)
{
    static bool ConvertSymbolicLinksToHard = true;
    static bool ConvertSymbolicLinksToCopies = true;    

    // Get source file name
    std::string file(path + "/" + name);
    char srcFile[1024];
    ssize_t fnLen = readlink(file.c_str(), srcFile, 1024);
    srcFile[fnLen] = 0;
    std::string sFile(srcFile);
    int res = 0;
    
    // if convert to hard is activated, and possible, do it
    if (ConvertSymbolicLinksToHard) {
        unlink(file.c_str());
        res = link(srcFile, file.c_str());
        if (res == 0) { return true; }
    }

    if ((errno == EXDEV) && ConvertSymbolicLinksToCopies) {
        copyfile(sFile, file);
    }
}

//----------------------------------------------------------------------
// Method: start
//----------------------------------------------------------------------
void DirWatcher::start()
{
    static std::string lastTriggerEventFileName("");

    static bool ConvertSymbolicLinks = true;

    int poll_num;
    nfds_t nfds;
    struct pollfd fds[1];

    char buf[4096]
        __attribute__ ((aligned(__alignof__(struct inotify_event))));

    // Initialize polling variables
    nfds = 1;
    fds[0].fd     = fd;
    fds[0].events = POLLIN;

    // Polling loop
    while (keepWatching) {

        poll_num = poll(fds, nfds, -1);
        if (poll_num == -1) {
            if (errno == EINTR) continue;
            perror("poll");
            exit(EXIT_FAILURE);
        }

        if ((poll_num > 0) && (fds[0].revents & POLLIN)) {

            // In case of events, get them and store them into the queue
            const struct inotify_event * event;
            ssize_t len;
            char * ptr;

            // Loop while events can be read from inotify file desc.
            for (;;) {
                // Read events
                len = read(fd, buf, sizeof buf);
                if ((len == -1) && (errno != EAGAIN)) {
                    perror("read");
                    exit(EXIT_FAILURE);
                }

                // If the nonblocking read() found no events to read, then it
                // returns -1 with errno set to EAGAIN. In that case, we exit
                // the loop
                if (len <= 0) break;

                // Loop over all events in the buffer
                for (ptr = buf; ptr < buf + len;
                     ptr += sizeof(struct inotify_event) + event->len) {

                    event = (const struct inotify_event *)(ptr);

                    // Store event in queue
                    DirWatchEvent dwe;
                    dwe.name = std::string(event->name);
                    if (dwe.name == lastTriggerEventFileName) { continue; }

                    dwe.path = watchedDirs[event->wd];
                    dwe.mask = event->mask;
                    dwe.isDir = (event->mask & IN_ISDIR);

                    lastTriggerEventFileName = dwe.name;

                    std::string file(dwe.path + "/" + dwe.name);
                    
                    // Check file size
                    if (!dwe.isDir) {
                        struct stat dweStat;
                        if (stat(file.c_str(), &dweStat) != 0) {
                            perror("stat");
//                             std::cerr << file << std::endl;
                            exit(EXIT_FAILURE);
                        } else {
                            dwe.size = dweStat.st_size;
                        }
                        // In case it is a symbolic link, and internally allowed,
                        // remove symbolic link and create hard link, or copy file
                        lstat(file.c_str(), &dweStat);
                        if (ConvertSymbolicLinks && S_ISLNK(dweStat.st_mode)) {
                            convertSymbolicLinks(dwe.path, dwe.name);
                        }

                        events.push(dwe);
                    }

                }
            }

        }
    }

    // Not watching anymore
    close(fd);
}

//----------------------------------------------------------------------
// Method: stop
//----------------------------------------------------------------------
void DirWatcher::stop()
{
    keepWatching = false;
}

//----------------------------------------------------------------------
// Method: nextEvent
//----------------------------------------------------------------------
bool DirWatcher::nextEvent(DirWatchEvent & event)
{
    if (events.size() > 0) {
        event = events.front();
        events.pop();
        return true;
    }
    return false;
}

