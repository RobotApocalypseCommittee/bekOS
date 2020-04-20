/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2020  Bekos Inc Ltd
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <printf.h>
#include <peripherals/gentimer.h>
#include <process/process.h>
#include <filesystem/fatfs.h>
#include <filesystem/entrycache.h>
#include <peripherals/emmc.h>
#include <filesystem/mbr.h>
#include <stdio.h>

extern FATFilesystem* filesystem;

extern "C"
void syscall_uart_write(char* str) {
    printf(str);
}

extern "C"
void syscall_delay_us(unsigned long us) {
    bad_udelay(us);
}

extern "C"
int syscall_get_pid() {
    return processManager.getCurrentProcess()->processID;
}

extern "C"
int syscall_open(char* path) {

    auto root = filesystem->getRootInfo();
    auto fileEntry = root->lookup(path);

    if (fileEntry.empty()) {
        // file doesn't exist
        return -1;
    }

    if (fileEntry->open == true) {
        for (int i = 0; i < processManager.getCurrentProcess()->openFiles.size(); i++) {
            if (processManager.getCurrentProcess()->openFiles[i] == fileEntry) {
                // file is already opened by this process, so go ahead
                return i;
            }
        }
        // file is opened by another process, do not go ahead
        return -1;
    }

    auto fp = filesystem->open(fileEntry);
    processManager.getCurrentProcess()->openFiles.push_back(fileEntry);
    fileEntry->open = true;

    return processManager.getCurrentProcess()->openFiles.size()-1;
}

extern "C"
int syscall_read(int index, char* buffer, int length) {

    if (index >=processManager.getCurrentProcess()->openFiles.size()) {
        return 0;
        // file of that index doesn't exist, no bytes read
    }

    auto fileEntry = processManager.getCurrentProcess()->openFiles[index];
    auto fp = filesystem->open(fileEntry);

    if (fileEntry->size > length) {
        fp->read(buffer, length, 0);
        return length;
        // reading not all of the file, so full length read
    }
    else {
        fp->read(buffer, fileEntry->size, 0);
        return fileEntry->size;
        // file is shorter than length, so full file read
    }
}

extern "C"
int syscall_close(int index) {

    auto root = filesystem->getRootInfo();

    if (index >= processManager.getCurrentProcess()->openFiles.size()) {
        return -1;
        // file is not open
    }

    // remove file from process's open files
    processManager.getCurrentProcess()->openFiles[index]->open = false;
    vector<AcquirableRef<FilesystemEntry>> newOpenFiles = {};
    for (int i = 0; i < processManager.getCurrentProcess()->openFiles.size(); i++) {
        if (i != index) {
            newOpenFiles.push_back(processManager.getCurrentProcess()->openFiles[i]);
        }
    }
    processManager.getCurrentProcess()->openFiles = newOpenFiles;

    // 0 = success
    return 0;
}

extern "C"
void * const syscall_table[] = {reinterpret_cast<void*>(syscall_uart_write),
                                reinterpret_cast<void*>(syscall_delay_us),
                                reinterpret_cast<void*>(syscall_get_pid),
                                reinterpret_cast<void*>(syscall_open),
                                reinterpret_cast<void*>(syscall_read),
                                reinterpret_cast<void*>(syscall_close)};
