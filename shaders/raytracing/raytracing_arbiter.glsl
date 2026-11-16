//the caller for raytracing pipeline
//remeber to #define MAX_DEPTH before include this file
struct RayState { // to hold the new direction
    vec3 ro,rd;
    float t;
    bool refracted; // caustic map
    bool inside_media; // to handle tranparent media
    float path_hash; //polgon splatting
    float wave_length; //380nm ~ 780nm
    float radiance; // 1.0f initially //L^λ
    int depth;
};

void onHit(RayState state,vec3 nd,float t);
void onMiss(RayState state);
// using the intersect function from scene, so the scene must be included before this file
void tracePath(inout RayState state){
    for(state.depth = 0; state.depth < MAX_DEPTH; state.depth++){
        Hit hit;
        if(intersect(ro,rd,t,state.wave_length)){
            onHit(state);
        }else{
            onMiss(state);
            break;
        }
    }
}