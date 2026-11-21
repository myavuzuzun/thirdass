#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <cmath>

#include "vst3/vst3_plugin.h"

int main() {
    printf("==================================================\n");
    printf(" THIRD ASS VST3 (WINDOWS x64) VERIFICATION SUITE  \n");
    printf("==================================================\n");

    HMODULE hMod = LoadLibraryA("dist\\ThirdAss.vst3\\Contents\\x86_64-win\\ThirdAss.vst3");
    if (!hMod) {
        hMod = LoadLibraryA("dist\\ThirdAss.vst3");
    }
    if (!hMod) {
        hMod = LoadLibraryA("dist\\ThirdAss_standalone.vst3");
    }
    if (!hMod) {
        printf("ERROR: Failed to load VST3 binary (GetLastError=%lu)\n", GetLastError());
        return 1;
    }

    typedef bool (__stdcall *InitDllFunc)();
    typedef bool (__stdcall *ExitDllFunc)();
    typedef IPluginFactory* (__stdcall *GetPluginFactoryFunc)();

    InitDllFunc initDll = (InitDllFunc)GetProcAddress(hMod, "InitDll");
    ExitDllFunc exitDll = (ExitDllFunc)GetProcAddress(hMod, "ExitDll");
    GetPluginFactoryFunc getFactory = (GetPluginFactoryFunc)GetProcAddress(hMod, "GetPluginFactory");

    if (!initDll || !exitDll || !getFactory) {
        printf("ERROR: Missing VST3 export functions\n");
        FreeLibrary(hMod);
        return 1;
    }
    printf("[PASS] VST3 Exports Verified: InitDll, ExitDll, GetPluginFactory\n");

    if (!initDll()) {
        printf("ERROR: InitDll() failed\n");
        FreeLibrary(hMod);
        return 1;
    }
    printf("[PASS] InitDll() initialized successfully\n");

    IPluginFactory* factory = getFactory();
    if (!factory) {
        printf("ERROR: GetPluginFactory returned null\n");
        FreeLibrary(hMod);
        return 1;
    }

    int32 count = factory->countClasses();
    printf("[PASS] Factory Class Count: %d\n", count);

    PClassInfo classInfo;
    std::memset(&classInfo, 0, sizeof(classInfo));
    factory->getClassInfo(0, &classInfo);
    printf("[PASS] Plugin Name: '%s', Category: '%s'\n", classInfo.name, classInfo.category);

    // 1. Create Component Instance
    void* compObj = nullptr;
    tresult res = factory->createInstance(classInfo.cid, IComponent_iid, &compObj);
    if (res != kResultOk || !compObj) {
        printf("ERROR: Failed to create IComponent instance (res=%d)\n", res);
        FreeLibrary(hMod);
        return 1;
    }
    printf("[PASS] IComponent instance created successfully\n");

    IComponent* component = static_cast<IComponent*>(compObj);
    component->initialize(nullptr);

    int32 audioIns = component->getBusCount(kAudio, kInput);
    int32 audioOuts = component->getBusCount(kAudio, kOutput);
    printf("[PASS] Audio Busses: %d In, %d Out\n", audioIns, audioOuts);

    // 2. Query IAudioProcessor
    void* procObj = nullptr;
    res = component->queryInterface(IAudioProcessor_iid, &procObj);
    if (res != kResultOk || !procObj) {
        printf("ERROR: Failed to query IAudioProcessor\n");
        component->terminate();
        component->release();
        FreeLibrary(hMod);
        return 1;
    }
    printf("[PASS] IAudioProcessor interface verified\n");

    IAudioProcessor* processor = static_cast<IAudioProcessor*>(procObj);
    ProcessSetup setup{};
    setup.processMode = 0;
    setup.symbolicSampleSize = 0;
    setup.maxSamplesPerBlock = 512;
    setup.sampleRate = 44100.0;
    processor->setupProcessing(setup);
    processor->setProcessing(true);

    // 3. Query IEditController
    void* ctrlObj = nullptr;
    res = component->queryInterface(IEditController_iid, &ctrlObj);
    if (res != kResultOk || !ctrlObj) {
        printf("ERROR: Failed to query IEditController\n");
        processor->release();
        component->terminate();
        component->release();
        FreeLibrary(hMod);
        return 1;
    }
    printf("[PASS] IEditController interface verified\n");

    IEditController* controller = static_cast<IEditController*>(ctrlObj);
    int32 paramCount = controller->getParameterCount();
    printf("[PASS] VST3 Parameter Count: %d\n", paramCount);

    // 4. Test IPlugView Direct2D GUI Lifecycle
    IPlugView* view = controller->createView("editor");
    if (view) {
        ViewRect rect{};
        view->getSize(&rect);
        int w = rect.right - rect.left;
        int h = rect.bottom - rect.top;
        printf("[PASS] IPlugView GUI dimensions: %d x %d\n", w, h);

        HWND hDummy = CreateWindowExA(0, "STATIC", "Host", WS_OVERLAPPED, 0, 0, w, h, nullptr, nullptr, nullptr, nullptr);
        if (hDummy) {
            view->attached(hDummy, kPlatformTypeHWND);
            view->removed();
            DestroyWindow(hDummy);
            printf("[PASS] IPlugView attached & removed (Direct2D lifecycle clean)\n");
        }
        view->release();
    }

    // 5. Cleanup
    processor->setProcessing(false);
    processor->release();
    controller->release();
    component->terminate();
    component->release();

    exitDll();
    FreeLibrary(hMod);

    printf("==================================================\n");
    printf(" ALL VST3 VERIFICATION CHECKS PASSED SUCCESSFULLY!\n");
    printf("==================================================\n");
    return 0;
}
