/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2024 Bekos Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "filesystem/filesystem.h"

#include "bek/utility.h"
#include "filesystem/block_device.h"
#include "filesystem/fatfs.h"
#include "filesystem/path.h"
#include "library/debug.h"

bek::shared_ptr<fs::FileHandle> fs::open_file(fs::EntryRef entry) {
    return bek::adopt_shared(new FileHandle{bek::move(entry)});
}
expected<fs::EntryRef> fs::fullPathLookup(EntryRef root, const fs::path& s, fs::EntryRef* out_parent) {
    if (s.is_absolute() || !root) {
        root = EXPECTED_TRY(FilesystemRegistry::the().lookup_root(s));
    }
    auto segments = s.segments();
    for (uSize i = 0; i < segments.size(); ++i) {
        if (!root->is_directory()) return ENOTDIR;

        auto segment = segments[i];
        // Special cases
        if (segment == "."_sv) continue;
        if (segment == ".."_sv) {
            if (root->parent()) {
                root = root->parent();
            }
            continue;
        }

        // Lookup
        auto lookup_result = root->lookup(segment);
        if (lookup_result.has_error()) {
            // We depart, but are obliged to provide parent if possible.
            if (out_parent && i + 1 == segments.size()) {
                *out_parent = root;
            }
            return lookup_result;
        } else {
            root = lookup_result.release_value();
        }
    }

    // We completed our mission
    if (out_parent) *out_parent = root->parent();
    return root;
}
expected<fs::EntryRef> fs::fullPathLookup(fs::EntryRef root, bek::str_view path, fs::EntryRef* out_parent) {
    bek::string path_s{path};
    auto path_r = EXPECTED_TRY(fs::path::parse_path(path_s));
    return fs::fullPathLookup(bek::move(root), path_r, out_parent);
}

fs::FilesystemRegistry* g_registry = nullptr;

void fs::FilesystemRegistry::register_filesystem(bek::string name, bek::own_ptr<Filesystem> fs) {
    if (g_registry == nullptr) {
        g_registry = new fs::FilesystemRegistry{bek::move(name), bek::move(fs)};
    } else {
        g_registry->m_filesystems.insert({bek::move(name), bek::move(fs)});
    }
}
fs::FilesystemRegistry::FilesystemRegistry(bek::string root_name, bek::own_ptr<Filesystem> root_fs)
    : m_root_filesystem{*root_fs} {
    VERIFY(root_fs);
    m_filesystems.insert({bek::move(root_name), bek::move(root_fs)});
}

fs::FilesystemRegistry& fs::FilesystemRegistry::the() {
    VERIFY(g_registry);
    return *g_registry;
}

expected<fs::EntryRef> fs::FilesystemRegistry::lookup_root(const fs::path& the_path) {
    if (!the_path.is_absolute()) return {EINVAL};

    if (the_path.disk_specifier()) {
        bek::string root_spec{*the_path.disk_specifier()};
        if (auto* fs = m_filesystems.find(root_spec); fs) {
            ASSERT(*fs);
            return (*fs)->get_root();
        } else {
            return {ENOENT};
        }
    } else {
        return m_root_filesystem.get_root();
    }
}
ErrorCode fs::FilesystemRegistry::try_mount_root() {
    auto devices = blk::BlockDeviceRegistry::the().get_accessible_devices();
    if (devices.size() == 0) return ENODEV;
    for (auto* dev : devices) {
        if (!dev) continue;
        auto mount_result = FATFilesystem::try_create_from(*dev);
        if (mount_result.has_error()) {
            if (mount_result.error() != EINVAL) return mount_result.error();
            continue;
        }
        fs::FilesystemRegistry::register_filesystem(bek::format("fat{}"_sv, dev->global_id()),
                                                    mount_result.release_value());
        return ESUCCESS;
    }
    return EINVAL;
}
