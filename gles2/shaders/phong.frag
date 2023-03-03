struct light_source {
  vec3 pos;
  float intense;
  float ambient;
  float attenuation;
};

vec3 phong_reflect(light_source light, vec3 surf_color, vec3 world_pos, vec3 world_normal) {
  vec3 light_direction = normalize(light.pos - world_pos);
  float brightnes = light.intense*dot(world_normal, light_direction);
  brightnes = clamp(brightnes, 0.0, 1.0);
  vec3 diffuse = brightnes*surf_color;
  vec3 ambient = light.intense*light.ambient*surf_color;
  float attenuation = 1.0/(1.0 + light.attenuation*pow(length(light.pos - world_pos), 2.0));
  return ambient + attenuation*diffuse;
}
