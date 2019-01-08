
typedef
struct pixel {
  unsigned char r, g, b;
  unsigned char alpha;
} pixel;

typedef
struct image {
  int width;
  int height;
  pixel ** data;
} image;

int image_create(image * img, int width, int height);
int image_write(image * pic, FILE * f);
int image_read(image * pic, FILE * f);
void image_bitblt(image* dest, image * src, int xof, int yof);
