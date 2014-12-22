/*
    * WinDBG Anti-RootKit extension
    * Copyright � 2013-2015  Vyacheslav Rusakoff
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
    * along with this program.  If not, see <http://www.gnu.org/licenses/>.

    * This work is licensed under the terms of the GNU GPL, version 3.  See
    * the COPYING file in the top-level directory.
*/

#include <direct.h>
#include <string>
#include <memory>
#include <fstream>
#include "resources.hpp"
#include "manipulators.hpp"

WDbgArkResHelper::WDbgArkResHelper() {
    m_main_subdir = "wdbgark";

    if ( g_Ext->IsCurMachine32() )
        m_platform_subdir = "x86";
    else
        m_platform_subdir = "x64";

    std::unique_ptr<char[]> tmp_path(new char[MAX_PATH + 1]);

    if ( GetTempPath(MAX_PATH + 1, tmp_path.get()) ) {
        m_temp_path = tmp_path.get();
        m_temp_path += (m_main_subdir + "\\");
        _mkdir(m_temp_path.c_str());
        m_temp_path += (m_platform_subdir + "\\");
        _mkdir(m_temp_path.c_str());
    }
}

bool WDbgArkResHelper::DropResource(const char* resource_name,
                                    const std::string &type,
                                    const std::string &file_name) {
    std::ofstream check_file_locked(m_temp_path + file_name, std::ios::out | std::ios::binary | std::ios::trunc);

    if ( check_file_locked.fail() )  // file already exists and locked by WinDbg
        return true;
    else
        check_file_locked.close();

    HRSRC resource = FindResource(g_Ext->s_Module, resource_name, type.c_str());

    if ( !resource ) {
        err << __FUNCTION__ << ": FindResource failed with error " << GetLastError() << endlerr;
        return false;
    }

    unsigned __int32 resource_size = SizeofResource(g_Ext->s_Module, resource);

    if ( !resource_size ) {
        err << __FUNCTION__ << ": SizeofResource failed with error " << GetLastError() << endlerr;
        return false;
    }

    HGLOBAL resource_data = LoadResource(g_Ext->s_Module, resource);

    if ( !resource_data ) {
        err << __FUNCTION__ << ": LoadResource failed with error " << GetLastError() << endlerr;
        return false;
    }

    void* data = LockResource(resource_data);

    if ( !data ) {
        err << __FUNCTION__ << ": LockResource failed" << endlerr;
        return false;
    }

    std::unique_ptr<char[]> error_msg(new char[MAX_PATH]);
    std::string file_path = m_temp_path + file_name;
    std::ofstream drop(file_path, std::ios::out | std::ios::binary | std::ios::trunc);

    if ( drop.fail() ) {
        strerror_s(error_msg.get(), MAX_PATH, errno);

        err << __FUNCTION__ << ": Error while creating file " << file_path << ", error is \"";
        err << error_msg.get() << "\"" << endlerr;
        return false;
    }

    drop.write(reinterpret_cast<char*>(data), resource_size);

    if ( drop.fail() ) {
        strerror_s(error_msg.get(), MAX_PATH, errno);

        err << __FUNCTION__ << ": Error while writing file " << file_path << ", error is \"";
        err << error_msg.get() << "\"" << endlerr;

        drop.close();
        return false;
    }

    drop.close();

    return true;
}