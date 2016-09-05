#include "AudioPluginUtil.h"
#include "SuperpoweredSimple.h"
#include "SuperpoweredSpatializer.h"

namespace Spatializer {
    static const float twoPi = float(2.0 * M_PI);
    static const float radianToDegrees = float(180.0 / M_PI);

    enum {
        P_SOUND2,
        P_OCCLUSION,
        P_NUM
    };

    struct SpatializerData {
        float parameters[P_NUM], sourcematrix[3], listenermatrix[15], lastDistanceIn, distanceMul;
        unsigned int lastSamplerate;
        SuperpoweredSpatializer *spatializer;
    };

    inline bool IsHostCompatible(UnityAudioEffectState* state) {
        return (state->structsize >= sizeof(UnityAudioEffectState)) && (state->hostapiversion >= UNITY_AUDIO_PLUGIN_API_VERSION);
    }

    int InternalRegisterEffectDefinition(UnityAudioEffectDefinition& definition) {
        int numparams = P_NUM;
        definition.paramdefs = new UnityAudioParameterDefinition[numparams];
        RegisterParameter(definition, "Alternative Sound", "", 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, P_SOUND2, "Alternative Sound");
        RegisterParameter(definition, "Occlusion", "", 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, P_OCCLUSION, "Occlusion");
        definition.flags |= UnityAudioEffectDefinitionFlags_IsSpatializer;
        return numparams;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK CreateCallback(UnityAudioEffectState *state) {
        SpatializerData *data = new SpatializerData;
        memset(data, 0, sizeof(SpatializerData));
        data->lastDistanceIn = data->distanceMul = 1.0f;
        data->lastSamplerate = state->samplerate;
        data->spatializer = new SuperpoweredSpatializer(state->samplerate);
        state->effectdata = data;
        InitParametersFromDefinitions(InternalRegisterEffectDefinition, data->parameters);
        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ReleaseCallback(UnityAudioEffectState *state) {
        SpatializerData *data = state->GetEffectData<SpatializerData>();
        delete data->spatializer;
        delete data;
        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK SetFloatParameterCallback(UnityAudioEffectState *state, int index, float value) {
        SpatializerData *data = state->GetEffectData<SpatializerData>();
        if (index >= P_NUM) return UNITY_AUDIODSP_ERR_UNSUPPORTED;
        switch (index) {
            case P_SOUND2: data->spatializer->sound2 = (value > 0.5f); break;
            case P_OCCLUSION: data->spatializer->occlusion = value; break;
        };
        data->parameters[index] = value;
        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK GetFloatParameterCallback(UnityAudioEffectState *state, int index, float *value, char *valuestr) {
        SpatializerData *data = state->GetEffectData<SpatializerData>();
        if (index >= P_NUM) return UNITY_AUDIODSP_ERR_UNSUPPORTED;
        if (value != NULL) *value = data->parameters[index];
        if (valuestr != NULL) valuestr[0] = 0;
        return UNITY_AUDIODSP_OK;
    }

    int UNITY_AUDIODSP_CALLBACK GetFloatBufferCallback(UnityAudioEffectState *state, const char *name, float *buffer, int numsamples) {
        return UNITY_AUDIODSP_OK;
    }

    UNITY_AUDIODSP_RESULT UNITY_AUDIODSP_CALLBACK ProcessCallback(UnityAudioEffectState *state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int outchannels) {
        if ((inchannels != 2) || (outchannels != 2) || !IsHostCompatible(state) || (state->spatializerdata == NULL)) {
            memcpy(outbuffer, inbuffer, (length * outchannels) << 2);
            return UNITY_AUDIODSP_OK;
        };
        SpatializerData *data = state->GetEffectData<SpatializerData>();

        if (state->samplerate != data->lastSamplerate) {
            data->lastSamplerate = state->samplerate;
            data->spatializer->setSamplerate(data->lastSamplerate);
        };

        bool matrixChanged = false;
        for (int n = 0; n < 3; n++) if (data->sourcematrix[n] != state->spatializerdata->sourcematrix[n + 12]) {
            matrixChanged = true;
            break;
        };
        if (!matrixChanged) {
            for (int n = 0; n < 15; n++) if (data->listenermatrix[n] != state->spatializerdata->listenermatrix[n]) {
                matrixChanged = true;
                break;
            }
        };
        if (matrixChanged) {
            memcpy(data->sourcematrix, &state->spatializerdata->sourcematrix[12], 12);
            memcpy(data->listenermatrix, state->spatializerdata->listenermatrix, 60);

            float *lm = state->spatializerdata->listenermatrix, *sm = state->spatializerdata->sourcematrix;
            float px = sm[12], py = sm[13], pz = sm[14]; // Currently we ignore source orientation and only use the position.
            float dir_x = lm[0] * px + lm[4] * py + lm[8] * pz + lm[12];
            float dir_y = lm[1] * px + lm[5] * py + lm[9] * pz + lm[13];
            float dir_z = lm[2] * px + lm[6] * py + lm[10] * pz + lm[14];
            float azimuth = (dir_z == 0.0f) ? 0.0f : atan2f(dir_x, dir_z);
            if (azimuth < 0.0f) azimuth += twoPi;
            azimuth *= radianToDegrees;
            if (azimuth < 0.0f) azimuth = 0.0f; else if (azimuth > 360.0f) azimuth = 360.0f;
            data->spatializer->azimuth = azimuth;
            float xz = sqrtf(dir_x * dir_x + dir_z * dir_z);
            data->spatializer->elevation = (atan2f(dir_y, xz) + 1e-10f) * radianToDegrees;
        };

        data->spatializer->reverbmix = state->spatializerdata->reverbzonemix;
        
        if (!data->spatializer->process(inbuffer, NULL, outbuffer, NULL, length, false)) memcpy(outbuffer, inbuffer, length << 3);
        return UNITY_AUDIODSP_OK;
    }
}
