
int Retro_SetColors(SDL_Surface *surface, SDL_Color *colors, int firstcolor, int ncolors)
{
   return 0;
}

int Retro_SetPalette(SDL_Surface * surface, int flags, const SDL_Color * colors,int firstcolor, int ncolors)
{

   SDL_Palette * palette= surface->format->palette;

   int status = 0;

   /* Verify the parameters */
   if (!palette)
      return -1;
   if (ncolors > (palette->ncolors - firstcolor))
   {
      ncolors = (palette->ncolors - firstcolor);
      status = -1;
   }

   if (colors != (palette->colors + firstcolor))
      memcpy(palette->colors + firstcolor, colors,
            ncolors * sizeof(*colors));
   ++palette->version;
   if (!palette->version)
      palette->version = 1;

   return status;
}


unsigned int Retro_MapRGB(SDL_PixelFormat *a, int r, int g, int b)
{
   if( a->BitsPerPixel == 16 )
      return RGB565( (r)>>3, (g)>>3, (b)>>3);
   else if( a->BitsPerPixel == 32 )
      return r<<16|g<<8|b;
   else if( a->BitsPerPixel == 8 )
      return 0;//TODO closest match in lut
   else return 0;
}

static const char *cross[] = {
   "X                               ",
   "XX                              ",
   "X.X                             ",
   "X..X                            ",
   "X...X                           ",
   "X....X                          ",
   "X.....X                         ",
   "X......X                        ",
   "X.......X                       ",
   "X........X                      ",
   "X.....XXXXX                     ",
   "X..X..X                         ",
   "X.X X..X                        ",
   "XX  X..X                        ",
   "X    X..X                       ",
   "     X..X                       ",
   "      X..X                      ",
   "      X..X                      ",
   "       XX                       ",
   "                                ",
};

void draw_cross(SDL_Surface * surf,int x,int y)
{
   int i,j,idx;
   int dx=32,dy=20;
   unsigned  color;

   for(j=y;j<y+dy;j++){
      idx=0;
      for(i=x;i<x+dx;i++){
         if(cross[j-y][idx]=='.')SDL_DrawPixel(surf,i,j,SDL_MapRGB(surf->format, 255, 255, 255));
         else if(cross[j-y][idx]=='X')SDL_DrawPixel(surf,i,j,SDL_MapRGB(surf->format, 0, 0, 0));
         idx++;			
      }
   }


}

void Retro_Fillrect(SDL_Surface * surf,SDL_Rect *rect,unsigned int col)
{
   if(rect==NULL)SDL_DrawFRect(surf,surf->clip_rect.x,surf->clip_rect.y,surf->clip_rect.w ,surf->clip_rect.h,col); 
   else SDL_DrawFRect(surf,rect->x,rect->y,rect->w ,rect->h,col); 
}

void Retro_FreeSurface(SDL_Surface *surf )
{   

   printf("free surf format palette color\n");
   if(surf->format->palette->colors)	
      free(surf->format->palette->colors);

   printf("free surf format palette \n");
   if(surf->format->palette)	
      free(surf->format->palette);
   printf("free surf format  \n");
   if(surf->format)	
      free(surf->format);
   printf("free surf pixel  \n"); 
   if(surf->pixels)
      free(surf->pixels);       
   printf("free surf  \n"); 
   if(surf)	
      free(surf);	

}

void Retro_BlitSurface(SDL_Surface *ss,SDL_Rect* sr,SDL_Surface *ds,SDL_Rect* dr)
{
   SDL_Rect src,dst;
   int x,y,w;
   unsigned char *pout = NULL;
   unsigned char *pin  = NULL;
   int sBPP=ss->format->BytesPerPixel;
   int dBPP=ds->format->BytesPerPixel;

   if(sr==NULL){
      src.x=0;
      src.y=0;
      src.w=ss->clip_rect.w;
      src.h=ss->clip_rect.h;
   }
   else {
      src.x=sr->x;
      src.y=sr->y;
      src.w=sr->w;
      src.h=sr->h;
   }

   if(dr==NULL){
      dst.x=0;
      dst.y=0;
      dst.w=ds->clip_rect.w;
      dst.h=ds->clip_rect.h;
   }
   else {
      dst.x=dr->x;
      dst.y=dr->y;
      dst.w=dr->w;
      dst.h=dr->h;
   }
   //printf("s(%d,%d,%d,%d) d(%d,%d,%d,%d)\n",src.x,src.y,src.w,src.h,dst.x,dst.y,dst.w,dst.h);
   pout=(unsigned char *)ds->pixels+(dst.x*dBPP+dst.y*ds->w*dBPP);
   pin =(unsigned char *)ss->pixels+(src.x*sBPP+src.y*ss->w*sBPP);
#if 1
   //COLORKEY
   if(ss->flags&SDL_SRCCOLORKEY)
   {
      unsigned int key=ss->format->colorkey;
      unsigned int res;

      for(y=0;y<src.h;y++)
      {
         for(x=0;x<src.w;x++)
         {
            if(sBPP==4)res=retro_unaligned32 (pin);
            else if(sBPP==2)res=retro_unaligned16 (pin);
            else  res=*pin;
            //FIXME 16 to 32 , 8 to 16, 8 to 32 & 32 to 16 . 
            if(sBPP==1 && dBPP==1)
            {
               if(key!=*pin)*pout=*pin;
               pout++;pin++;
            }
            else if(sBPP==1 && dBPP==4)
            {
               unsigned int mcoul=ss->format->palette->colors[*pin].r<<16|\
                                  ss->format->palette->colors[*pin].g<<8|\
                                  ss->format->palette->colors[*pin].b;
               //printf("c:%x- ",mcoul);
	       retro_unaligned32 (pout) = mcoul;
	       pout += 4;
               pin++;
            }
            else
            {
               for(w=0;w<dBPP;w++)
               {	
                  if(res!=key)
                     *pout=*pin;
                  pout++;pin++;
               }
            }

         }
         pin +=(ss->w-src.w)*sBPP;
         pout+=(ds->w-src.w)*dBPP;
      }
      return;
   }
#endif
   for(y=0;y<src.h;y++)
   {
      for(x=0;x<src.w;x++)
      {

         //FIXME 16 to 32 , 8 to 16, 8 to 32 & 32 to 16 .
         if(sBPP==1 && dBPP==1)
         {
            *pout=*pin;
            pout++;pin++;
         }
         else 
         {
            if(sBPP==1 && dBPP==4)
            {
               unsigned int mcoul=ss->format->palette->colors[*pin].r<<16|\
                                  ss->format->palette->colors[*pin].g<<8|\
                                  ss->format->palette->colors[*pin].b;
               //printf("c:%x- ",mcoul);
	       retro_unaligned32 (pout) = mcoul;
	       pout += 4;
               pin++;
            }
            else
            {
               for(w=0;w<dBPP;w++)
               {
                  *pout=*pin;
                  pout++;pin++;
               }
            }
         }
      }
      pin +=(ss->w-src.w)*sBPP;
      pout+=(ds->w-src.w)*dBPP;
   }
}

int Retro_SetColorKey(SDL_Surface *surface, Uint32 flag, Uint32 key)
{
   if(flag==SDL_SRCCOLORKEY)
   {
      surface->format->colorkey=key;
      surface->flags|=SDL_SRCCOLORKEY;
   }

   return 0;
}


SDL_Surface *Retro_CreateRGBSurface( int w,int h, int d, int rm,int rg,int rb,int ra)
{
   SDL_Surface *bitmp = (SDL_Surface *) calloc(1, sizeof(*bitmp));

   if (bitmp == NULL)
   {
      printf("tex surface failed");
      return NULL;
   }

   bitmp->format = calloc(1,sizeof(*bitmp->format));
   if (bitmp->format == NULL)
   {
      printf("tex format failed");
      return NULL;
   }

   bitmp->format->palette = calloc(1,sizeof(*bitmp->format->palette));
   if (bitmp->format->palette == NULL)
   {
      printf("tex format palette failed");
      return NULL;
   }
   printf("create palette\n");
   bitmp->format->palette->ncolors=256;
   bitmp->format->palette->colors=malloc(1024);
   bitmp->format->palette->version=0;
   bitmp->format->palette->refcount=0;
   memset(bitmp->format->palette->colors,0x0,1024);
   printf("fill surf %d \n",bitmp->format->palette->colors[255].r);
   if(d==16)
   {
      bitmp->format->BitsPerPixel = 16;
      bitmp->format->BytesPerPixel = 2;
      bitmp->format->Rloss=3;
      bitmp->format->Gloss=3;
      bitmp->format->Bloss=3;
      bitmp->format->Aloss=0;
      bitmp->format->Rshift=11;
      bitmp->format->Gshift=6;
      bitmp->format->Bshift=0;
      bitmp->format->Ashift=0;
      bitmp->format->Rmask=0x0000F800;
      bitmp->format->Gmask=0x000007E0;
      bitmp->format->Bmask=0x0000001F;
      bitmp->format->Amask=0x00000000;
      bitmp->format->colorkey=0;
      bitmp->format->alpha=0;
      //bitmp->format->palette = NULL;

      bitmp->flags=0;
      bitmp->w=w;
      bitmp->h=h;
      bitmp->pitch=/*retro*/w*2;
      bitmp->pixels=malloc(sizeof(unsigned char)*h*w*2);//  (unsigned char *)&Retro_Screen[0];
      if (!bitmp->pixels)
      {
         printf("failed alloc pixels\n");	
         Retro_FreeSurface(bitmp);            
         return NULL;
      }
      memset(bitmp->pixels,0, h*w*2);
   }
   else if(d==32)
   {
      bitmp->format->BitsPerPixel = 32;
      bitmp->format->BytesPerPixel = 4;
      bitmp->format->Rloss=0;
      bitmp->format->Gloss=0;
      bitmp->format->Bloss=0;
      bitmp->format->Aloss=0;
      bitmp->format->Rshift=16;
      bitmp->format->Gshift=8;
      bitmp->format->Bshift=0;
      bitmp->format->Ashift=24;
      bitmp->format->Rmask=0x00ff0000;
      bitmp->format->Gmask=0x0000ff00;
      bitmp->format->Bmask=0x000000ff;
      bitmp->format->Amask=0xff000000;
      bitmp->format->colorkey=0;
      bitmp->format->alpha=0;
      //bitmp->format->palette = NULL;

      bitmp->flags=0;
      bitmp->w=w;
      bitmp->h=h;
      bitmp->pitch=w*4;
      bitmp->pixels=malloc(sizeof(unsigned char)*h*w*4);//  (unsigned char *)&Retro_Screen[0];
      if (!bitmp->pixels)
      {
         printf("failed alloc pixels\n");	
         Retro_FreeSurface(bitmp);            
         return NULL;
      }
      memset(bitmp->pixels,0, h*w*4);
   }
   else
   {
      bitmp->format->BitsPerPixel = 8;
      bitmp->format->BytesPerPixel = 1;
      bitmp->format->Rloss=0;
      bitmp->format->Gloss=0;
      bitmp->format->Bloss=0;
      bitmp->format->Aloss=0;
      bitmp->format->Rshift=0;
      bitmp->format->Gshift=0;
      bitmp->format->Bshift=0;
      bitmp->format->Ashift=0;
      bitmp->format->Rmask=0;
      bitmp->format->Gmask=0;
      bitmp->format->Bmask=0;
      bitmp->format->Amask=0;
      bitmp->format->colorkey=0;
      bitmp->format->alpha=0;
      bitmp->flags=0;
      bitmp->w=w;
      bitmp->h=h;
      bitmp->pitch=w*1;
      bitmp->pixels=malloc(sizeof(unsigned char)*h*w*1);//  (unsigned char *)&Retro_Screen[0];
      if (!bitmp->pixels)
      {
         printf("failed alloc pixels\n");	
         Retro_FreeSurface(bitmp);            
         return NULL;
      }
      memset(bitmp->pixels,0, h*w*1);
   }

   bitmp->clip_rect.x=0;
   bitmp->clip_rect.y=0;
   bitmp->clip_rect.w=w;
   bitmp->clip_rect.h=h;

   //printf("fin prepare tex:%dx%dx%d\n",bitmp->w,bitmp->h,bitmp->format->BytesPerPixel);
   return bitmp;
}

SDL_Surface *Retro_SetVideoMode(int w,int h,int b)
{
   SDL_Surface *bitmp;

   bitmp = (SDL_Surface *) calloc(1, sizeof(*bitmp));
   if (bitmp == NULL)
   {
      printf("tex surface failed");
      return NULL;
   }

   bitmp->format = calloc(1,sizeof(*bitmp->format));
   if (bitmp->format == NULL)
   {
      printf("tex format failed");
      return NULL;
   }


   bitmp->format->palette = calloc(1,sizeof(*bitmp->format->palette));
   if (bitmp->format->palette == NULL)
   {
      printf("tex format palette failed");
      return NULL;
   }
   printf("create palette\n");
   bitmp->format->palette->ncolors=256;
   bitmp->format->palette->colors=malloc(1024);
   bitmp->format->palette->version=0;
   bitmp->format->palette->refcount=0;
   memset(bitmp->format->palette->colors,0x0,1024);

   if(b==16)
   {
      bitmp->format->BitsPerPixel = 16;
      bitmp->format->BytesPerPixel = 2;
      bitmp->format->Rloss=3;
      bitmp->format->Gloss=3;
      bitmp->format->Bloss=3;
      bitmp->format->Aloss=0;
      bitmp->format->Rshift=11;
      bitmp->format->Gshift=6;
      bitmp->format->Bshift=0;
      bitmp->format->Ashift=0;
      bitmp->format->Rmask=0x0000F800;
      bitmp->format->Gmask=0x000007E0;
      bitmp->format->Bmask=0x0000001F;
      bitmp->format->Amask=0x00000000;
      bitmp->format->colorkey=0;
      bitmp->format->alpha=0;
      //bitmp->format->palette = NULL;

      bitmp->flags=0;
      bitmp->w=w;
      bitmp->h=h;
      bitmp->pitch=retrow*2;
      bitmp->pixels=(unsigned char *)&Retro_Screen[0];
      bitmp->clip_rect.x=0;
      bitmp->clip_rect.y=0;
      bitmp->clip_rect.w=w;
      bitmp->clip_rect.h=h;
   }
   else
   {
      bitmp->format->BitsPerPixel = 32;
      bitmp->format->BytesPerPixel = 4;
      bitmp->format->Rloss=0;
      bitmp->format->Gloss=0;
      bitmp->format->Bloss=0;
      bitmp->format->Aloss=0;
      bitmp->format->Rshift=16;
      bitmp->format->Gshift=8;
      bitmp->format->Bshift=0;
      bitmp->format->Ashift=24;
      bitmp->format->Rmask=0x00ff0000;
      bitmp->format->Gmask=0x0000ff00;
      bitmp->format->Bmask=0x000000ff;
      bitmp->format->Amask=0xff000000;
      bitmp->format->colorkey=0;
      bitmp->format->alpha=0;
      //bitmp->format->palette = NULL;
      bitmp->flags=0;
      bitmp->w=w;
      bitmp->h=h;
      bitmp->pitch=retrow*4;
      bitmp->pixels=(unsigned char *)&Retro_Screen[0];
      bitmp->clip_rect.x=0;
      bitmp->clip_rect.y=0;
      bitmp->clip_rect.w=w;
      bitmp->clip_rect.h=h;
   }

   printf("SetvideoMode fin prepare tex:%dx%dx%d pitch:%d\n",bitmp->w,bitmp->h,bitmp->format->BytesPerPixel,bitmp->pitch);
   return bitmp;
}      

void Retro_GetRGB(int coul,SDL_PixelFormat *format, int *r,int *g,int *b)
{
   if(format->BitsPerPixel==16)
   {
#define R5 ((coul>>11)&0x1F)
#define G6 ((coul>>5 )&0x3F)
#define B5 ((coul    )&0x1F)

      *r = ( R5 * 527 + 23 ) >> 6;
      *g = ( G6 * 259 + 33 ) >> 6;
      *b = ( B5 * 527 + 23 ) >> 6;
   }
   else
   {
#define R ((coul>>16)&0xff)
#define G ((coul>>8 )&0xff)
#define B ((coul    )&0xff)
      *r = R;
      *g = G;
      *b = B;
   }
}
