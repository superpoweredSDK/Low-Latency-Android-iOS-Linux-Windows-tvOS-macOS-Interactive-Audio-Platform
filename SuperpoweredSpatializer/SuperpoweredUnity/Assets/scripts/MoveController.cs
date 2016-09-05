using UnityEngine;
using System.Collections;

public class MoveController : MonoBehaviour
{
    public float wrapLen = 40.0f;
    public Vector3 speed = new Vector3(0, 0, 2.5f);

    Vector3 startPos;

    void Start()
    {
        startPos = transform.position;
    }

    void Update()
    {
        var cam = GetComponent<Camera>();
        if (cam == null)
            return;

        transform.Translate(speed * Time.deltaTime);
        var delta = transform.position - startPos;
        if (delta.magnitude > wrapLen)
            transform.position = startPos;
    }
}
