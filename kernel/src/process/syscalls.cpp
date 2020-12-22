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

extern fs::Filesystem *filesystem;

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
    return processManager->getCurrentProcess()->processID;
}

extern "C"
int syscall_open(char* path) {

    char *upperPath = path;
    for (int i = 0; i < strlen(path); i++) {
        // if it's a lower case letter, make it upper case, otherwise, who cares
        if ((int) path[i] > 98 && (int) path[i] < 123) {
            upperPath[i] = (char) ((int) path[i] - 32);
        } else {
            upperPath[i] = path[i];
        }
    }

    auto root = filesystem->getRootInfo();
    auto fileEntry = fs::fullPathLookup(root, upperPath);

    if (fileEntry.empty()) {
        // file doesn't exist
        return -1;
    }

    if (fileEntry->open) {
        for (uSize i = 0; i < processManager->getCurrentProcess()->openFiles.size(); i++) {
            if (&processManager->getCurrentProcess()->openFiles[i]->getEntry() == fileEntry.get()) {
                // file is already opened by this process, so go ahead
                return i;
            }
        }
        // file is opened by another process, do not go ahead
        return -1;
    }

    auto fp = filesystem->open(fileEntry);
    for (uSize i = 0; i < processManager->getCurrentProcess()->openFiles.size(); i++) {
        // Find empty spaces
        if (processManager->getCurrentProcess()->openFiles[i] == nullptr) {
            processManager->getCurrentProcess()->openFiles[i] = fp;
            return i;
        }
    }
    processManager->getCurrentProcess()->openFiles.push_back(fp);

    return processManager->getCurrentProcess()->openFiles.size()-1;
}

extern "C"
unsigned long syscall_read(unsigned long index, char* buffer, unsigned long length) {

    if (index >= processManager->getCurrentProcess()->openFiles.size()) {
        return 0;
        // file of that index doesn't exist, no bytes read
    }

    auto fp = processManager->getCurrentProcess()->openFiles[index];
    if (fp == nullptr) return 0;

    auto &fentry = fp->getEntry();

    if (fentry.size > length) {
        fp->read(buffer, length, 0);
        // reading not all of the file, so full length read
        return length;
    } else {
        fp->read(buffer, fentry.size, 0);
        // file is shorter than length, so full file read
        return fentry.size;
    }
}

extern "C"
int syscall_close(unsigned long index) {

    auto root = filesystem->getRootInfo();

    if (index >= processManager->getCurrentProcess()->openFiles.size()) {
        // file is not open
        return -1;
    }

    // remove file from process's open files
    fs::File *fp = processManager->getCurrentProcess()->openFiles[index];
    processManager->getCurrentProcess()->openFiles[index] = nullptr;
    fp->close();
    delete fp;
    return 0;
}

extern "C"
void * const syscall_table[] = {reinterpret_cast<void*>(syscall_uart_write),
                                reinterpret_cast<void*>(syscall_delay_us),
                                reinterpret_cast<void*>(syscall_get_pid),
                                reinterpret_cast<void*>(syscall_open),
                                reinterpret_cast<void*>(syscall_read),
                                reinterpret_cast<void*>(syscall_close)};
