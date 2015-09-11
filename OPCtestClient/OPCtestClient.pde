OPC opc;

void setup() {
  opc = new OPC(this, "pixels.local", 7890);
  frameRate(30);
  for(int i=0; i<64; i++) opc.setPixel(i, 0);
}

int   i = 0;
color c = 0xFF0000;

void draw() {
  opc.setPixel(i, c);
  if(i >= 4) opc.setPixel(i-4, 0);
  opc.writePixels();

  if(++i > 67) {
    i = 0;
    if((c >>= 8) == 0) c = 0xFF0000;
  }
}
