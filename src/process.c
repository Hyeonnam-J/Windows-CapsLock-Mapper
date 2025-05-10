#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "constants/app_constants.h"
#include "constants/key_constants.h"
#include "constants/result_constants.h"
#include "utils/common.h"
#include "utils/mutex.h"
#include "utils/env.h"
#include "utils/registry.h"

#include "process.h"

int on_runner() {
    int response = is_mutex_exist(MUTEX_KEY_RUNNER);
    if (response == MUTEX_FOUND) {
        printf(MUTEX_FOUND_MESSAGE);
    } else {
        char path[MAX_PATH];
        int currentPathResult = get_current_path(path, APP_CMD);
        
        if (currentPathResult != SUCCESS) {
            if  (currentPathResult == ERR_GET_EXE_PATH) printf(ERR_GET_EXE_PATH_MESSAGE);
            else if (currentPathResult == ERR_CUTPOINT_NOT_FOUND) printf(ERR_CUTPOINT_NOT_FOUND_MESSAGE);

            return currentPathResult;
        } 

        strcat(path, APP_RUNNER);

        char command[MAX_PATH + 20];
        snprintf(command, sizeof(command), "start /B %s", path);
        system(command);

        printf("Program has started.\n");
    }

    return SUCCESS;
}

int off_runner() {
    int response = is_mutex_exist(MUTEX_KEY_RUNNER);
    if (response == MUTEX_FOUND) {
        char command[MAX_PATH + 20];
        snprintf(command, sizeof(command), "taskkill /F /IM %s >nul 2>&1", APP_RUNNER);
        system(command);
        printf("Program has stopped.\n");
    } else {
        printf("Program was not running.\n");
    }
    
    return SUCCESS;
}

int show_status() {
    // 1. Check Mutex.
    int mutex_response = is_mutex_exist(MUTEX_KEY_RUNNER);

    // 2. Check Env.
    // Get env path.
    HKEY hKey_env;
    DWORD size = 0;
    char* envPath;

    int envPathResult = get_env_path(&hKey_env, &size, &envPath);
    if (envPathResult != SUCCESS) {
        if (envPathResult == ERR_REG_KEY_OPEN) printf(ERR_REG_KEY_OPEN_MESSAGE);
        else if (envPathResult == ERR_ENV_QUERY_SIZE) printf(ERR_ENV_QUERY_SIZE_MESSAGE);
        else if (envPathResult == ERR_MEMORY_ALLOCATION) printf(ERR_MEMORY_ALLOCATION_MESSAGE);
        else if (envPathResult == ERR_GET_ENVIRONMENT_VAR) printf(ERR_GET_ENVIRONMENT_VAR_MESSAGE);

        RegCloseKey(hKey_env);
        free(envPath);

        return envPathResult;
    }

    // Get Exe dir path.
    char exeDirPath[MAX_PATH];
    int currentPathResult = get_current_path(exeDirPath, APP_CMD);
    strcat(exeDirPath, ";");
    
    if (currentPathResult != SUCCESS) {
        if  (currentPathResult == ERR_GET_EXE_PATH) printf(ERR_GET_EXE_PATH_MESSAGE);
        else if (currentPathResult == ERR_CUTPOINT_NOT_FOUND) printf(ERR_CUTPOINT_NOT_FOUND_MESSAGE);

        free(envPath);
        RegCloseKey(hKey_env);

        return currentPathResult;
    } 
    // Remember call free(envPath) & RegCloseKey(hKey_env)

    // 3. Check Registry.
    int registry_response = is_registry_exist();
    if (registry_response == ERR_REG_KEY_OPEN) {
        free(envPath);
        RegCloseKey(hKey_env);

        printf(ERR_REG_KEY_OPEN_MESSAGE);
        return ERR_REG_KEY_OPEN;
    }

    // 4. print.
    printf("[Running status]    %s\n", mutex_response == MUTEX_FOUND ? "Running." : "Not running.");
    printf("[Env status]        %s\n", strstr(envPath, exeDirPath) == NULL ? "Not found." : "Found.");
    printf("[Registry status]   %s\n", registry_response == REGISTRY_FOUND ? "Found." : "Not found.");
    
    // 5. clean.
    free(envPath);
    RegCloseKey(hKey_env);
    return SUCCESS;
}

int add_env() {
    HKEY hKey;
    DWORD size = 0;
    char* envPath = NULL;

    int envPathResult = get_env_path(&hKey, &size, &envPath);
    if (envPathResult != SUCCESS) {
        if (envPathResult == ERR_REG_KEY_OPEN) printf(ERR_REG_KEY_OPEN_MESSAGE);
        else if (envPathResult == ERR_ENV_QUERY_SIZE) printf(ERR_ENV_QUERY_SIZE_MESSAGE);
        else if (envPathResult == ERR_MEMORY_ALLOCATION) printf(ERR_MEMORY_ALLOCATION_MESSAGE);
        else if (envPathResult == ERR_GET_ENVIRONMENT_VAR) printf(ERR_GET_ENVIRONMENT_VAR_MESSAGE);

        free(envPath);
        RegCloseKey(hKey);

        return envPathResult;
    }

    char exeDirPath[MAX_PATH];
    int currentPathResult = get_current_path(exeDirPath, APP_CMD);

    if (currentPathResult != SUCCESS) {
        if  (currentPathResult == ERR_GET_EXE_PATH) printf(ERR_GET_EXE_PATH_MESSAGE);
        else if (currentPathResult == ERR_CUTPOINT_NOT_FOUND) printf(ERR_CUTPOINT_NOT_FOUND_MESSAGE);

        free(envPath);
        RegCloseKey(hKey);

        return currentPathResult;
    } 
    
    strcat(exeDirPath, ";");
    if (strstr(envPath, exeDirPath) == NULL) {
        size_t envLen = strlen(envPath);
        if (envLen > 0 && envPath[envLen - 1] != ';') {
            strcat(envPath, ";");
        }
        strcat(envPath, exeDirPath);

        if (RegSetValueEx(hKey, "Path", 0, REG_SZ, (BYTE*)envPath, strlen(envPath) + 1) != ERROR_SUCCESS) {
            free(envPath);
            RegCloseKey(hKey);
            printf(ERR_SET_ENVIRONMENT_VAR_MESSAGE);
            return ERR_SET_ENVIRONMENT_VAR;
        }

        SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
        printf("Environment path has been updated successfully.\n");
    } else {
        printf("The environment path already includes this executable.\n");
    }
    
    free(envPath);
    RegCloseKey(hKey);
    return SUCCESS;
}

int remove_env() {
    HKEY hKey;
    DWORD size = 0;
    char* envPath = NULL;

    int envPathResult = get_env_path(&hKey, &size, &envPath);
    if (envPathResult != SUCCESS) {
        if (envPathResult == ERR_REG_KEY_OPEN) printf(ERR_REG_KEY_OPEN_MESSAGE);
        else if (envPathResult == ERR_ENV_QUERY_SIZE) printf(ERR_ENV_QUERY_SIZE_MESSAGE);
        else if (envPathResult == ERR_MEMORY_ALLOCATION) printf(ERR_MEMORY_ALLOCATION_MESSAGE);
        else if (envPathResult == ERR_GET_ENVIRONMENT_VAR) printf(ERR_GET_ENVIRONMENT_VAR_MESSAGE);

        free(envPath);
        RegCloseKey(hKey);
        return envPathResult;
    }

    char exeDirPath[MAX_PATH];
    int currentPathResult = get_current_path(exeDirPath, APP_CMD);

    if (currentPathResult != SUCCESS) {
        if  (currentPathResult == ERR_GET_EXE_PATH) printf(ERR_GET_EXE_PATH_MESSAGE);
        else if (currentPathResult == ERR_CUTPOINT_NOT_FOUND) printf(ERR_CUTPOINT_NOT_FOUND_MESSAGE);

        free(envPath);
        RegCloseKey(hKey);
        return currentPathResult;
    } 

    char* pathStart = strstr(envPath, exeDirPath);

    if (pathStart != NULL) {
        size_t pathLen = strlen(exeDirPath);
        memmove(pathStart, pathStart + pathLen, strlen(pathStart + pathLen) + 1);

        size_t envLen = strlen(envPath);
        if (envLen > 0 && envPath[envLen - 1] == ';') {
            envPath[envLen - 1] = '\0';  
        }

        if (RegSetValueEx(hKey, "Path", 0, REG_SZ, (BYTE*)envPath, strlen(envPath) + 1) != ERROR_SUCCESS) {
            free(envPath);
            RegCloseKey(hKey);
            printf(ERR_SET_ENVIRONMENT_VAR_MESSAGE);
            return ERR_SET_ENVIRONMENT_VAR;
        }

        SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
        printf("Environment path has been removed successfully.\n");
    } else {
        printf("The specified path is not found in the environment variable.\n");
    }

    free(envPath);
    RegCloseKey(hKey);
    return SUCCESS;
}

int add_registry() {
    int response = is_registry_exist();
    if (response == ERR_REG_KEY_OPEN) {
        printf(ERR_REG_KEY_OPEN_MESSAGE);
        return ERR_REG_KEY_OPEN;
    } else if (response == REGISTRY_FOUND) {
        printf("Already registered.\n");
        return SUCCESS;
    } 
    
    // response == REGISTRY_NOT_FOUND
    
    char exeDirPath[MAX_PATH];
    int currentPathResult = get_current_path(exeDirPath, APP_CMD);

    if (currentPathResult != SUCCESS) {
        if  (currentPathResult == ERR_GET_EXE_PATH) printf(ERR_GET_EXE_PATH_MESSAGE);
        else if (currentPathResult == ERR_CUTPOINT_NOT_FOUND) printf(ERR_CUTPOINT_NOT_FOUND_MESSAGE);

        return currentPathResult;
    } 

    strcat(exeDirPath, APP_RUNNER);

    HKEY hKey;
    LONG openResult = RegOpenKeyEx(HKEY_CURRENT_USER, REG_PATH, 0, KEY_SET_VALUE, &hKey);

    if (openResult != ERROR_SUCCESS) {
        printf(ERR_REG_KEY_OPEN_MESSAGE);
        return ERR_REG_KEY_OPEN;
    }

    LONG setResult = RegSetValueEx(hKey, APP_NAME, 0, REG_SZ, (BYTE*)exeDirPath, strlen(exeDirPath) + 1);
    RegCloseKey(hKey);
    
    if (setResult == ERROR_SUCCESS) {
        printf("Registry successfully registered.\n");
        return SUCCESS;
    } else {
        printf(ERR_SET_REGISTRY_MESSAGE);
        return ERR_SET_REGISTRY;
    }
}

int remove_registry() {
    int response = is_registry_exist();
    if (response == ERR_REG_KEY_OPEN) {
        printf(ERR_REG_KEY_OPEN_MESSAGE);
        return ERR_REG_KEY_OPEN;
    } else if (response == REGISTRY_NOT_FOUND) {
        printf("Already removed.\n");
        return SUCCESS;
    }

    // response == REGISTRY_FOUND

    HKEY hKey;
    LONG regResult = RegOpenKeyEx(HKEY_CURRENT_USER, REG_PATH, 0, KEY_SET_VALUE, &hKey);

    if (regResult != ERROR_SUCCESS) {
        printf(ERR_REG_KEY_OPEN_MESSAGE);
        return ERR_REG_KEY_OPEN;
    }

    regResult = RegDeleteValue(hKey, APP_NAME);
    RegCloseKey(hKey);

    if (regResult == ERROR_SUCCESS) {
        printf("Registry entry removed successfully.\n");
        return SUCCESS;
    } else {
        printf(ERR_DELETE_REGISTRY_MESSAGE);
        return ERR_DELETE_REGISTRY;
    }
}

int show_version() {
    printf("%s version %s\n", APP_NAME, VERSION);
    return SUCCESS;
}

int show_help() {
    printf("Usage:\n");
    printf("  on                    - Start %s\n", APP_RUNNER);
    printf("  off                   - Terminate %s\n", APP_RUNNER);
    printf("  env --add             - Add the current user path to the environment\n");
    printf("  env --remove          - Remove the current user path from the environment\n");
    printf("  registry --add        - Register %s to run on startup (registry)\n", APP_RUNNER);
    printf("  registry --remove     - Remove %s from startup registry\n", APP_RUNNER);
    printf("  status                - Show operating status\n");
    printf("  --version             - Show version info\n");
    printf("  --help                - Show this help message\n");

    return SUCCESS;
}

int show_help_invalid() {
    printf("Invalid command. Use '--help' to see available options.\n");
    return SUCCESS;
}

struct CommandWithOptions commandWithOptions[] = {
    {
        .command = {
            "on",
            on_runner
        },
        .options = {
            { NULL, NULL, NULL }
        }
    },
    {
        .command = {
            "off",
            off_runner
        },
        .options = {
            { NULL, NULL, NULL }
        }
    },
        {
        .command = {
            "status",
            show_status
        },
        .options = {
            { NULL, NULL, NULL }
        }
    },
    {
        .command = {
            NULL,
            NULL
        },
        .options = {
            { "--help", "-h", show_help },
            { "--version", "-v", show_version },
            { NULL, NULL, NULL }
        }
    },
    {
        .command = {
            "env",
            NULL
        },
        .options = {
            { "--add", "-a", add_env },
            { "--remove", "-r", remove_env },
            { NULL, NULL, NULL }
        }
    },
    {
        .command = {
            "registry",
            NULL
        },
        .options = {
            { "--add", "-a", add_registry },
            { "--remove", "-r", remove_registry },
            { NULL, NULL, NULL }
        }
    },
    {
        .command = {
            NULL,
            NULL
        },
        .options = {
            { NULL, NULL, NULL }
        }
    },
};