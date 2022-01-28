/*
Decals as per FoGED Ch 10.1

Different to projected textures:
- Decals have a bounding box.
- Decals are meshes created based on geometry they intersect with.
- This is determined based on clipping of geometry with the decal's BB.

Algorithm:
A) create 6 planes of decal's BB
2. inputs are centre position of box P, and normal vector N.
3. choose a tangent vector T to be perp to N (e.g. Listing 9.10).
   tangent can have random rotation (varied splats) or aligned to something (e.g. for footprints).
4. complete right-handed coordinate system B = NxT. Normalise all 3 vectors if necessary.
5. decide on decal's BB height +-d. Make large enough to absorb bumps in surface, but not so big
   that decal will be applied to distant surfaces unintentionally.
6. determine 6 inward-facing planes
   r_x and r_y are radius of decal on x,y.
   f_left   =  [  T | r_x - T dot P ]    f_right =  [ -T | r_x + T dot P ]
   f_bottom =  [  B | r_y - B dot P ]    f_top   =  [ -B | r_y + B dot P ]
   f_back   =  [  N | d   - N dot P ]    f_front =  [ -N | d   + N dot P ]
B) build the mesh for the decal.
1. search the world for other geometries that intersect with BB
2. for each geometry: clip its triangles against all 6 planes of BB using method described in Sec 9.1
   - transform BB into object space of the geometry being intersected.
   - discard clipped triangles
   - remaining triangles form decal
   - using CONSISTENT vertex ordering so maintain exact edge coordinates for new vertices and avoid seams/gaps.
   - 
*/
