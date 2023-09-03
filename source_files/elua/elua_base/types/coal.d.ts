
/** @noSelfInFile */



declare namespace sys {

    // native
    const gametic: number;

    function error(s: string);
    function print(s: string);
    function debug_print(s: string);

    function edge_version(): number;

    const TICRATE: number;
}


declare namespace math {

    const e: number;
    //const pi: number; <--- also defined in math.d.ts for lua module

    // native 
    function rint(val: number): number;
    function floor(val: number): number;
    function ceil(val: number): number;

    function cos(val: number): number;
    function sin(val: number): number;
    function tan(val: number): number;
    function log(val: number): number;

    function acos(val: number): number;
    function asin(val: number): number;
    function atan(val: number): number;
    function atan2(x: number, y: number): number;

    function random(): number;
    function random2(): number;

    function abs(val: number): number;
    function sqrt(val: number): number;
    function min(a: number, b: number): number;
    function max(a: number, b: number): number;

    function rand_range(low: number, high: number): number;

    // Vec3 extenstions
    function getx(vec: Vec3): number;
    function gety(vec: Vec3): number;
    function getz(vec: Vec3): number;

    function vlen(vec: Vec3): number;
    function normalize(vec: Vec3): Vec3;

}



