/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "systimemgr.h"
#include <fstream>
#include <map>
#include <cstdlib>
#include "irdklog.h"
#include <iostream>
#include <telemetry_busmessage_sender.h>

int main()
{
    try {
        string debugcfg("/etc/debug.ini");
        rdk_logger_init(debugcfg.c_str());
        SysTimeMgr* sysTimeMgr = SysTimeMgr::get_instance();

        sysTimeMgr->initialize();
        sysTimeMgr->run();
    }
    catch (const std::bad_cast& e) {
        std::cerr << "Caught std::bad_cast exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception& e) {
        std::cerr << "Caught std::exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "Caught unknown exception." << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
