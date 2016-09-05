#include "AudioPluginUtil.h"
#include "SuperpoweredSpatializer.h"

namespace SpatializerReverb {
    static SuperpoweredSpatializerGlobalReverb globalReverb;
    
    enum {
        P_ROOMSIZE,
        P_DAMP,
        P_NUM
    };

    struct SpatializerReverbData {
        float parameters[P_NUM];
        unsigned int lastSamplerate;
    };

    int InternalRegisterEffectDefinition(UnityAudioEffectDefinition &definition) {
        int numparams = P_NUM;
        definition.paramdefs = new UnityAudioParameterDefinition[numparams];
        RegisterParameter(definition, "Room Size", "", 0.0f, 1.0f, 0.5f, 1.0f, 1.0f, P_ROOMSIZE, "Room Size");
        RegisterParameter(definition, "Damp", "", 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, P_DAMP, "Damp");
        return numparams;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK CreateCallback(UnityAudioEffectState *state) {
        SpatializerReverbData *data = new SpatializerReverbData;
        memset(data, 0, sizeof(SpatializerReverbData));
        data->lastSamplerate = 44100;
        state->effectdata = data;
        SuperpoweredSpatializerGlobalReverb::setReverbSamplerate(state->samplerate);
        InitParametersFromDefinitions(InternalRegisterEffectDefinition, data->parameters);
        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ReleaseCallback(UnityAudioEffectState *state) {
        SpatializerReverbData *data = state->GetEffectData<SpatializerReverbData>();
        delete data;
        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK SetFloatParameterCallback(UnityAudioEffectState *state, int index, float value) {
        SpatializerReverbData *data = state->GetEffectData<SpatializerReverbData>();
        if (index >= P_NUM) return UNITY_AUDIODSP_ERR_UNSUPPORTED;
        switch (index) {
            case P_ROOMSIZE: SuperpoweredSpatializerGlobalReverb::reverb->setRoomSize(value); break;
            case P_DAMP: SuperpoweredSpatializerGlobalReverb::reverb->setDamp(value); break;
        };
        data->parameters[index] = value;
        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK GetFloatParameterCallback(UnityAudioEffectState *state, int index, float *value, char *valuestr) {
        SpatializerReverbData *data = state->GetEffectData<SpatializerReverbData>();
        if (index >= P_NUM) return UNITY_AUDIODSP_ERR_UNSUPPORTED;
        if (value != NULL) *value = data->parameters[index];
        if (valuestr != NULL) valuestr[0] = 0;
        return UNITY_AUDIODSP_OK;
    }

    int UNITY_AUDIODSP_CALLBACK GetFloatBufferCallback(UnityAudioEffectState *state, const char *name, float *buffer, int numsamples) {
        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ProcessCallback(UnityAudioEffectState *state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int outchannels) {
        if ((inchannels != 2) || (outchannels != 2)) {
            memset(outbuffer, 0, (length * outchannels) << 2);
            return UNITY_AUDIODSP_OK;
        }
        SpatializerReverbData *data = state->GetEffectData<SpatializerReverbData>();

        if (state->samplerate != data->lastSamplerate) {
            data->lastSamplerate = state->samplerate;
            SuperpoweredSpatializerGlobalReverb::reverb->setSamplerate(data->lastSamplerate);
        };

        if (!SuperpoweredSpatializerGlobalReverb::process(outbuffer, length)) memset(outbuffer, 0, (length * outchannels) << 2);
        return UNITY_AUDIODSP_OK;
    }
}
