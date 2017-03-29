RWTexture2D<float4> outImage : register(u0);                                                           
                                                                                                      
cbuffer Parameters : register(b0)                                                                     
{                                                                                                     
    uint  width;                                                                                      
    uint  height;                                                                                     
    uint  gridWidth;                                                                                  
    uint  lineWidth;                                                                                  
    float4 colorBg;                                                                                   
    float4 colorFg;                                                                                   
};                                                                                                    
                                                                                                      
// The number of threads to be executed in a single thread group when a compute shader is dispatched  
[numthreads(8, 8, 1)]                                                                                 
void main(uint3 coord : SV_DispatchThreadID)                                                          
{         
	
    if((coord.x < width) && (coord.y < height))                                                       
    {                                                                                                 
		float4 colorOut = colorBg;
		uint  x = coord.x % gridWidth;
		uint  y = coord.y % gridWidth;

		if (x < lineWidth || y < lineWidth)
		{
            colorOut = colorFg;
        }
        outImage[coord.xy] = colorOut;
    }
}