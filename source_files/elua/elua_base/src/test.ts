


const v3 = vec3();

sys.print(sys.TICRATE.toString());
v3[0] = 67;
v3[1] = 68;
v3[2] = 69;
sys.print(v3[0].toString());

sys.print(math.toString());

const nv3 = math.normalize(v3);
sys.print(math.vlen(nv3).toString());
sys.print(math.getx(nv3).toString());
sys.print(math.gety(nv3).toString());
sys.print(math.getz(nv3).toString());


/*
class Hud {

    say_hello(num: number) {
        ec.sys.print("hello world!", num)
    }

}


export const hud = new Hud();
export default hud;

ec.sys.print(hud, sys.TICRATE);
*/
