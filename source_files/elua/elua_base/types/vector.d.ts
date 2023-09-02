

declare type Vec2 = [number, number] & {
    (x?: number, y?: number): Vec2;
    add: LuaAddition<Vec2, Vec2, Vec2>;
}

declare type Vec3 = [number, number] & {
    (x?: number, y?: number, z?: number): Vec3;
    add: LuaAddition<Vec3, Vec3, Vec3>;
}
