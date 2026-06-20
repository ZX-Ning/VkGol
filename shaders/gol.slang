[[vk_binding(0, 0)]]
RWStructuredBuffer<int8_t> src;

[[vk_binding(1, 0)]]
RWStructuredBuffer<int8_t> target;

struct Shape {
    uint X;
    uint Y;
};

[[vk_push_constant]]
ConstantBuffer<Shape> shape;

[[ForceInline]]
int get(uint x, uint y) {
    return (x >= 0 && x < shape.X && y >= 0 && y < shape.Y) ? src[y * shape.X + x] : 0;
}

[[ForceInline]]
void set(uint x, uint y, int value) {
    if (x >= 0 && x < shape.X && y >= 0 && y < shape.Y) {
        target[y * shape.X + x] = (int8_t)value;
    }
}

[[shader("compute")]]
[[numthreads(16, 16, 1)]]
void update(uint2 id: SV_DispatchThreadID) {
    static const int2[8] MOVES = {
        { 0, 1 },
        { 0, -1 },
        { 1, 0 },
        { -1, 0 },
        { 1, 1 },
        { 1, -1 },
        { -1, -1 },
        { -1, 1 }
    };
    int neighbours = 0;
    [[ForceUnroll]]
    for (int i = 0; i < 8; i++) {
        int2 mov = MOVES[i];
        neighbours += get(id.x + mov.x, id.y + mov.y);
    }
    int currect = get(id.x, id.y);
    if (currect > 0 && (neighbours < 2 || neighbours > 3)) {
        set(id.x, id.y, 0);
    }
    else if (currect == 0 && neighbours == 3) {
        set(id.x, id.y, 1);
    }
    else {
        set(id.x, id.y, currect);
    }
}


