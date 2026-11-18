// Coordinates measured on the exported Meshy strap, in game model metres.
// The belt passes through the upper loop, not the model bounding-box centre.
namespace lantern_fit {
constexpr std::array<float,3> strapAnchor={-.005f,-.012f,-.032f};
// In-game feedback on 0.1.8 established: +local Z is forward, -local Z rearward.
// Move the right-side attachment 45 degrees toward the front (+local Z),
// retaining its 22 cm radius and height. Rotate the whole fitting with it.
constexpr std::array<float,3> beltAnchor={.025f,-.15556349f,.15556349f};
// Pelvis compensation + inward-facing strap + 12 degrees inward lean,
// then -45 degrees around pelvis local X (the character's vertical axis).
// Rotation around the strap keeps its belt contact fixed as the body tilts.
constexpr std::array<float,4> rotation={.33740219f,.20082918f,-.67798794f,.62141754f};
inline std::array<float,3> rotate(const std::array<float,3>& p) {
 const auto& q=rotation;
 const std::array<float,3> t={2*(q[1]*p[2]-q[2]*p[1]),2*(q[2]*p[0]-q[0]*p[2]),2*(q[0]*p[1]-q[1]*p[0])};
 return {p[0]+q[3]*t[0]+q[1]*t[2]-q[2]*t[1],
         p[1]+q[3]*t[1]+q[2]*t[0]-q[0]*t[2],
         p[2]+q[3]*t[2]+q[0]*t[1]-q[1]*t[0]};
}
inline std::array<float,3> offset() {
 const auto p=rotate(strapAnchor);
 return {beltAnchor[0]-p[0],beltAnchor[1]-p[1],beltAnchor[2]-p[2]};
}
}
