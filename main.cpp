#include <windows.h>
#include <vector>
#include <cmath>
#include <fstream>
#include "C_API.h"

//kad se projekat kreira -> create cpp project using existing resources
//za pokretanje -> Release mod, a ne Debug u  konfiguraciji i obrisati ako ima L"TEXT" -> "TEXT"
// i wchar_t promeniti u char

std::ofstream camera_position_file("camera_positions.txt");

bool updated = true;

enum class Mode {
    EXPLORE, PICTURE
};

Mode mode = Mode::EXPLORE;

BITMAPINFO bmi;
HBITMAP hBitmap;
HDC hMemDC;
HBITMAP oldBitmap;
COLORREF *pBitmapData;

LARGE_INTEGER frequency;
LARGE_INTEGER renderStart, renderEnd, displayStart, displayEnd, frameStart, frameEnd;
double renderTime = 0.0;
double displayTime = 0.0;

POINT windowCenter;

void initBitmap(HWND hwnd) {
    hMemDC = CreateCompatibleDC(GetDC(hwnd));

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = CAMERA_WIDTH;
    bmi.bmiHeader.biHeight = -CAMERA_HEIGHT; // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;

    hBitmap = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, (void **)&pBitmapData, NULL, 0);
    oldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
}

void render() {
    QueryPerformanceCounter(&renderStart);

    renderer_render();

    for (int y = 0; y < CAMERA_HEIGHT; ++y) {
        for (int x = 0; x < CAMERA_WIDTH; ++x) {
            vec3 color = renderer_canvas[CAMERA_HEIGHT - 1 - y][x];
            pBitmapData[y * CAMERA_WIDTH + x] = RGB(color.z >> 8, color.y >> 8, color.x >> 8);
        }
    }

    QueryPerformanceCounter(&renderEnd);
    renderTime = double(renderEnd.QuadPart - renderStart.QuadPart) / frequency.QuadPart;
}

void snapPicture(HWND hwnd) {
    mode = Mode::PICTURE;
    for (int y = 0; y < CAMERA_HEIGHT; ++y) {
        for (int x = 0; x < CAMERA_WIDTH; ++x) {
            real u = (x - 400) << 7;
            real v = (CAMERA_HEIGHT - 1 - y - 300) << 7;
            vec3 temp = camera_get_ray_direction(u, v);
            ray ray;
            ray.origin.x = camera_position->x;
            ray.origin.y = camera_position->y;
            ray.origin.z = camera_position->z;
            ray.direction.x = temp.x;
            ray.direction.y = temp.y;
            ray.direction.z = temp.z;
            vec3 color = scene_trace_ray(&ray, 0);
            pBitmapData[y * CAMERA_WIDTH + x] = RGB(color.z >> 8, color.y >> 8, color.x >> 8);
        }
    }
    InvalidateRect(hwnd, NULL, FALSE);
    UpdateWindow(hwnd);
}

void display(HWND hwnd) {
    QueryPerformanceCounter(&displayStart);

    HDC hdc = GetDC(hwnd);
    BitBlt(hdc, 0, 0, CAMERA_WIDTH, CAMERA_HEIGHT, hMemDC, 0, 0, SRCCOPY);
    ReleaseDC(hwnd, hdc);

    QueryPerformanceCounter(&displayEnd);
    displayTime = double(displayEnd.QuadPart - displayStart.QuadPart) / frequency.QuadPart;
}

real SPEED_YAW = 0x00008000;
real SPEED_PITCH = 0x00008000;
real SPEED_MOVE = real_mul(REAL_PI, SPEED_YAW);

real speed_move, speed_yaw, speed_pitch;
int tab_pressed = 0;
int esc_pressed = 0;

POINT last_mouse_pos;
bool right_mouse_pressed = false;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_MOUSEMOVE:
    {
        if (mode != Mode::EXPLORE) {
            break;
        }
        // Handle mouse movement to adjust camera
        POINT currentMousePos;
        GetCursorPos(&currentMousePos);

        real deltaX = (currentMousePos.x - windowCenter.x) << 7;
        real deltaY = (currentMousePos.y - windowCenter.y) << 7;

        real yaw_change = -real_mul(deltaX, SPEED_YAW);
        real pitch_change = -real_mul(deltaY, SPEED_PITCH);

        *camera_yaw += yaw_change;
        *camera_pitch += pitch_change;

        if (*camera_yaw < -REAL_PI_TWO) *camera_yaw = REAL_PI_TWO;
        if (*camera_yaw > REAL_PI_TWO) *camera_yaw = -REAL_PI_TWO;
        if (*camera_pitch < -REAL_PI_TWO) *camera_pitch = +REAL_PI_TWO;
        if (*camera_pitch > REAL_PI_TWO) *camera_pitch = -REAL_PI_TWO;

        camera_update();

        last_mouse_pos = currentMousePos;

        SetCursorPos(windowCenter.x, windowCenter.y);

        updated = true;
        break;
    }
    case WM_RBUTTONUP:
    {
        if (mode != Mode::EXPLORE) {
            break;
        }
        if (!*scene_ghost_mode) {
            right_mouse_pressed = true;
            *scene_ghost_mode = true;
            *scene_ghost_radius = SCENE_GHOST_RADIUS_INIT;
            *scene_ghost_radius2 = SCENE_GHOST_RADIUS_INIT2;
            updated = true;
        }
        else {
            *scene_ghost_mode = false;
            vec3 c = *camera_position, forward = *camera_forward;
            forward = vec3_multiply_scalar(&forward, SCENE_SPAWN_DISTANCE + *scene_ghost_radius);
            c = vec3_add(&c, &forward);
            scene_add_sphere(&c, *scene_ghost_radius);
        }
        break;
    }
    case WM_LBUTTONUP:
    {
        // shoot ray and the first sphere it hits (if any) is marked

        real min_t = 0x7fffffff;
        sphere *min_s = nullptr;
        for (int i = 0; i < *scene_num_of_spheres; i++) {
            real t;
            ray r;
            r.origin.x = camera_position->x;
            r.origin.y = camera_position->y;
            r.origin.z = camera_position->z;
            r.direction.x = camera_forward->x;
            r.direction.y = camera_forward->y;
            r.direction.z = camera_forward->z;
            if (sphere_intersect(&scene_spheres[i], &r, &t)) {
                if (t < min_t) {
                    min_t = t;
                    min_s = &scene_spheres[i];
                }
            }
        }
        *scene_marked_sphere_address = min_s;
        updated = true;
        break;
    }
    case WM_MOUSEWHEEL:
    {
        if (*scene_ghost_mode) {
            int mouse_delta = GET_WHEEL_DELTA_WPARAM(wParam);
            const real radius_delta = 0x00010000 >> 5;
            if (mouse_delta > 0) {
                *scene_ghost_radius += radius_delta;
                *scene_ghost_radius2 = real_mul(*scene_ghost_radius, *scene_ghost_radius);
            }
            else {
                *scene_ghost_radius -= radius_delta;
                if (*scene_ghost_radius < SCENE_GHOST_RADIUS_MIN) {
                    *scene_ghost_radius = SCENE_GHOST_RADIUS_MIN;
                    *scene_ghost_radius2 = SCENE_GHOST_RADIUS_MIN2;
                }
            }
            updated = true;
        }
        break;
    }
    case WM_PAINT:
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        display(hwnd);
        EndPaint(hwnd, &ps);
        return 0;
    case WM_DESTROY:
        SelectObject(hMemDC, oldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemDC);
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

bool escape_initialized = true;

void exploreMode(HWND hwnd, real delta_time) {
    speed_move -= real_mul(delta_time, SPEED_MOVE) >> 1;
    speed_yaw -= real_mul(delta_time, SPEED_YAW) >> 1;
    speed_pitch -= real_mul(delta_time, SPEED_PITCH) >> 1;

    vec3 temp_vec3; // helper for vec3 operations
    real temp_real; // helper for real operations

    if (GetAsyncKeyState('W') & 0x8000) {
        temp_real = real_mul(SPEED_MOVE, delta_time);
        temp_vec3 = vec3_multiply_scalar(camera_forward, temp_real);
        *camera_position = vec3_add(camera_position, &temp_vec3);
        updated = true;
    }
    if (GetAsyncKeyState('S') & 0x8000) {
        temp_real = real_mul(SPEED_MOVE, delta_time);
        temp_vec3 = vec3_multiply_scalar(camera_forward, temp_real);
        *camera_position = vec3_subtract(camera_position, &temp_vec3);
        updated = true;
    }
    if (GetAsyncKeyState('A') & 0x8000) {
        temp_real = real_mul(SPEED_MOVE, delta_time);
        temp_vec3 = vec3_multiply_scalar(camera_right, temp_real);
        *camera_position = vec3_subtract(camera_position, &temp_vec3);
        updated = true;
    }
    if (GetAsyncKeyState('D') & 0x8000) {
        temp_real = real_mul(SPEED_MOVE, delta_time);
        temp_vec3 = vec3_multiply_scalar(camera_right, temp_real);
        *camera_position = vec3_add(camera_position, &temp_vec3);
        updated = true;
    }
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
        camera_position->y += real_mul(SPEED_MOVE, delta_time);
        updated = true;
    }
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
        camera_position->y -= real_mul(SPEED_MOVE, delta_time);
        if (camera_position->y < 0x00001000) {
            camera_position->y = 0x00001000;
        }
        updated = true;
    }
    if (GetAsyncKeyState(VK_DELETE) & 0x8000) {
        scene_remove_sphere(*scene_marked_sphere_address);
        *scene_marked_sphere_address = nullptr;
        updated = true;
    }
    if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
        snapPicture(hwnd);  
        return;
    }
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        if (!esc_pressed) {
            PostQuitMessage(0);
            esc_pressed = 1;
        }
        return;
    }
    else {
        esc_pressed = 0;
    }
    if (GetAsyncKeyState(VK_TAB) & 0x8000) {
        if (!tab_pressed) {
            updated = true;
            material_light_mode = !material_light_mode;
        }
        tab_pressed = 1;
    }
    else {
        tab_pressed = 0;
    }

    if (updated) {
        render();
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
        updated = false;
    }
}

void pictureMode(HWND hwnd, real delta_time) {
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        if (!esc_pressed) {
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
            updated = true;
            mode = Mode::EXPLORE;
            esc_pressed = 1;
        }
        else {
            esc_pressed = 0;
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    material_init_colors();

    const wchar_t CLASS_NAME[] = L"RayTracingDemo";
    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Ray Tracing Demo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CAMERA_WIDTH, CAMERA_HEIGHT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    initBitmap(hwnd);

    scene_reset();

    QueryPerformanceFrequency(&frequency);

    RECT windowRect;
    GetClientRect(hwnd, &windowRect);
    windowCenter.x = (windowRect.right - windowRect.left) / 2;
    windowCenter.y = (windowRect.bottom - windowRect.top) / 2;
    ClientToScreen(hwnd, &windowCenter); // Convert to screen coordinates

    SetCursorPos(windowCenter.x, windowCenter.y); // Set cursor to window center
    ShowCursor(FALSE); // Hide the cursor
    SetCapture(hwnd);  // Capture the mouse input

    MSG msg = {};
    QueryPerformanceCounter(&frameStart);
    for (unsigned long long i = 0; true; i++) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                return 0;
            }
        }

        QueryPerformanceCounter(&frameEnd);
        uint32_t highFrequency = (1 << 16) * double(frameEnd.QuadPart - frameStart.QuadPart) / frequency.QuadPart;
        real delta_time = highFrequency;
        frameStart = frameEnd;

        switch (mode) {
        case Mode::EXPLORE:
            exploreMode(hwnd, delta_time);
            break;
        case Mode::PICTURE:
            pictureMode(hwnd, delta_time);
            break;
        }
    }

    return 0;
}
