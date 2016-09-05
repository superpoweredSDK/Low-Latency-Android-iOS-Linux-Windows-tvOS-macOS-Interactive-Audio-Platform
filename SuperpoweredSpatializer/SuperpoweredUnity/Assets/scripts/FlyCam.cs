using UnityEngine;
using System.Collections;

public class FlyCam : MonoBehaviour
{
    public float lookSpeed = 5.0f;
    public float moveSpeed = 1.0f;

    public float rotationX = 0.0f;
    public float rotationY = 0.0f;

    void Update()
    {
        if (Input.GetMouseButton(0))
        {
            rotationX += Input.GetAxis("Mouse X") * lookSpeed;
            while (rotationX >= 360.0f) rotationX -= 360.0f;
            while (rotationX <= -360.0f) rotationX += 360.0f;
            rotationY += Input.GetAxis("Mouse Y") * lookSpeed;
            if (rotationY < -90.0f) rotationY = -90.0f;
            else if (rotationY > 90.0f) rotationY = 90.0f;
        }

        transform.localRotation  = Quaternion.AngleAxis(rotationX, Vector3.up);
        transform.localRotation *= Quaternion.AngleAxis(rotationY, Vector3.left);

        transform.position += transform.forward * moveSpeed * Input.GetAxis("Vertical");
        transform.position += transform.right * moveSpeed * Input.GetAxis("Horizontal");
        transform.position += transform.up * 3 * moveSpeed * Input.GetAxis("Mouse ScrollWheel");
    }
}
