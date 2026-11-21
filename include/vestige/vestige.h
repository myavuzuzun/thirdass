#ifndef VESTIGE_H
#define VESTIGE_H

/*
 * VeSTige: Open-Source Clean-Room Audio Plugin Header
 * Originally created by Javier Serrano Polo and the LMMS Development Team.
 * Provides binary-level plugin compatibility without any proprietary SDK dependencies.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VESTIGE_MAGIC 0x56737450 // 'VstP'

enum vestige_opcodes {
    effOpen = 0,
    effClose = 1,
    effSetProgram = 2,
    effGetProgram = 3,
    effSetProgramName = 4,
    effGetProgramName = 5,
    effGetParamLabel = 6,
    effGetParamDisplay = 7,
    effGetParamName = 8,
    effSetSampleRate = 10,
    effSetBlockSize = 11,
    effMainsChanged = 12,
    effEditGetRect = 13,
    effEditOpen = 14,
    effEditClose = 15,
    effEditIdle = 19,
    effGetChunk = 23,
    effSetChunk = 24,
    effProcessEvents = 25,
    effGetProgramNameIndexed = 29,
    effGetEffectName = 45,
    effGetVendorString = 47,
    effGetProductString = 48,
    effGetVendorVersion = 49,
    effCanDo = 51,
    effGetTailSize = 52,
    effGetParameterProperties = 56,
    effGetVstVersion = 58
};

enum vestige_flags {
    effFlagsHasEditor = 1 << 0,
    effFlagsCanReplacing = 1 << 4,
    effFlagsProgramChunks = 1 << 5,
    effFlagsIsSynth = 1 << 8,
    effFlagsCanDoubleReplacing = 1 << 12
};

struct ERect {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
};

struct _AEffect;
typedef struct _AEffect AEffect;

typedef intptr_t (*audioMasterCallback)(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);
typedef intptr_t (*dispatcher_t)(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);
typedef void (*process_t)(AEffect* effect, float** inputs, float** outputs, int32_t sampleframes);
typedef void (*processDouble_t)(AEffect* effect, double** inputs, double** outputs, int32_t sampleframes);
typedef void (*setParameter_t)(AEffect* effect, int32_t index, float parameter);
typedef float (*getParameter_t)(AEffect* effect, int32_t index);

struct _AEffect {
    int32_t magic;
    dispatcher_t dispatcher;
    process_t process;
    setParameter_t setParameter;
    getParameter_t getParameter;
    int32_t numPrograms;
    int32_t numParams;
    int32_t numInputs;
    int32_t numOutputs;
    int32_t flags;
    intptr_t resvd1;
    intptr_t resvd2;
    int32_t initialDelay;
    int32_t realQualities;
    int32_t offQualities;
    float ioRatio;
    void* object;
    void* user;
    int32_t uniqueID;
    int32_t version;
    process_t processReplacing;
    processDouble_t processDoubleReplacing;
    char future[56];
};

#ifdef __cplusplus
}
#endif

#endif // VESTIGE_H
