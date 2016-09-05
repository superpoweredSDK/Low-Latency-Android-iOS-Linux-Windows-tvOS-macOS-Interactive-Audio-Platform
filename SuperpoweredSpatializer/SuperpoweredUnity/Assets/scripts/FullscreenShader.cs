using UnityEngine;
using System.Collections;

public class FullscreenShader : MonoBehaviour
{
    public Material mat;

    void Start()
    {
    }

    void Update()
    {
    }

    void OnRenderImage(RenderTexture src, RenderTexture dest)
    {
        Graphics.Blit(src, dest, mat);
    }
}
