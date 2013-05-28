#define CHANGE_X 1
#define CHANGE_Z 2
#define CHANGE_Y 4
#define EPSILON 0.000000001f

__constant int xSize = 8;
__constant int ySize = 8;
__constant int zSize = 8;
__constant float d = 32.f;
__constant float dt = 0.1f;
__constant float xMax = 40;
__constant float zMax = 40;
__constant float envDensity = 0.1f;

int lin(int x, int y, int z);
float poly6(float r);
float3 spiky_grad(float r, float3 r_vec);
float viscosity_laplace(float r);
int boundaries(float3 position);

__kernel 
void fluids(
    __global float3 * initialVelocity, //0
    __global float3 * initialPosition, //1
    __constant float * mass, //2
    __global float * density, //3
    __global float * pressure, //4
    __constant float * gasConstant, //5
    __constant float * mu, //6
    __constant float3 * gravity, //7
    __global float3 * finalVelocity, //8
    __global float3 * finalPosition) //9
{
    int xIdx = get_global_id(0);
    int yIdx = get_global_id(1);
    int zIdx = get_global_id(2);
    int me = lin(xIdx, yIdx, zIdx);
    __private float3 currentPosition = initialPosition[lin(xIdx, yIdx, zIdx)];
    __private float3 currentVelocity = initialVelocity[lin(xIdx, yIdx, zIdx)];
    //printf("Vel: (%f, %f, %f)\n", currentVelocity.x, currentVelocity.y, currentVelocity.z);
    
    __private float3 nextVelocity = {0, 0, 0};
    __private float3 nextPosition = {0, 0, 0};
    __private float currentPressure = 0;
    __private float3 f_pressure = {0, 0, 0};
    __private float3 f_viscosity = {0, 0, 0};
    __private float our_mu = * mu;
    
    /*let's find the density.
    rho(x) = sum_j (m_j W(abs(x - x_j)))
    it's the sum of all the nearby masses multiplied by the smoothing function.
    */
    __private float currentLocalDensity;
    currentLocalDensity = EPSILON;
    for (int i = 0; i < xSize * ySize * zSize; i++) {
        currentLocalDensity += mass[i] * poly6(fast_distance(currentPosition, initialPosition[i])   );
        //printf("%f ", mass[i]);
    }
    //printf("CLD: %.15f\n", currentLocalDensity);
    density[me] = currentLocalDensity;
    
    currentPressure = *gasConstant * (currentLocalDensity - envDensity);
    pressure[me] = currentPressure;
    //printf("CLP: %.15f\n", currentPressure);
    barrier(CLK_GLOBAL_MEM_FENCE);
    
    __private float their_mass = 0;
    __private float3 their_position = {0, 0, 0};
    __private float their_pressure = 0;
    __private float their_density = 0;
    __private float3 their_velocity = {0, 0, 0};
    __private float dis = 0;
    for (int i = 0; i < xSize * ySize * zSize; i++) {
        their_mass = mass[i];
        their_position = initialPosition[i];
        their_pressure = pressure[i];
        their_density = density[i];
        dis = fast_distance(currentPosition, their_position) + EPSILON;
        f_pressure -= their_mass * (currentPressure + their_pressure) / (2 * their_density) * spiky_grad(dis, currentPosition - their_position);
        /*f_pressure.x -= their_mass * (currentPressure + their_pressure) / (2 * their_density) * spiky_grad(
            dis
        );
        f_pressure.y -= their_mass * (currentPressure + their_pressure) / (2 * their_density) * spiky_grad(
            dis
        );
        f_pressure.z -= their_mass * (currentPressure + their_pressure) / (2 * their_density) * spiky_grad(
            dis
        );*/      
        
        f_viscosity.x += their_mass * (their_velocity.x - currentVelocity.x) / their_density * viscosity_laplace(
            dis
        );
        //printf("%f ", f_viscosity.x);//Debug viscosity in x
        //printf("%f ", their_density);
        f_viscosity.y += their_mass * (their_velocity.y - currentVelocity.y) / their_density * viscosity_laplace(
            dis
        );
        //printf("%f ", f_viscosity.y);//Debug viscosity in y
        f_viscosity.z += their_mass * (their_velocity.z - currentVelocity.z) / their_density * viscosity_laplace(
            dis
        );
        //printf("Vis: (%f, %f, %f)\n", f_viscosity.x, f_viscosity.y, f_viscosity.z);
        //printf("Delta_Vel: (%f, %f, %f)\n", their_velocity.x - currentVelocity.x, their_velocity.y - currentVelocity.y, their_velocity.z - currentVelocity.z);//Debug viscosity in z
    }
    /*printf("Pressure: (%f, %f, %f)\n",
        f_pressure.x,
        f_pressure.y,
        f_pressure.z);*/
        
    //printf("Viscosity: (%f, %f, %f)\n", f_viscosity.x, f_viscosity.y, f_viscosity.z);
    f_viscosity *= our_mu;
    
    __private float3 f_gravity;
    f_gravity = *gravity * currentLocalDensity;
    
    //combine the forces and stuff
    __private float3 f_total;
    f_total = f_pressure + f_gravity + f_viscosity;
    nextVelocity = currentVelocity + f_total / currentLocalDensity * dt;
    
    nextPosition = currentPosition + 0.5f * (nextVelocity + currentVelocity) * dt;
    
    __private int boundaryChange;
    boundaryChange = boundaries(nextPosition);
    if (boundaryChange & CHANGE_X) {
        nextVelocity.x *= -1;
    }
    if (boundaryChange & CHANGE_Y) {
        nextVelocity.y *= -1;
    }
    if (boundaryChange & CHANGE_Z) {
        nextVelocity.z *= -1;
    }
    
    finalVelocity[me] = nextVelocity;
    finalPosition[me] = nextPosition;    
}

int lin(int x, int y, int z) {
    return z * xSize * ySize + xSize * y + x;
}

float poly6(float r) {
    if (r >= 0 && r <= d) {
        return 315 * pow(pow(d, 2) - pow(r, 2), 3) /(64 * M_PI_F * pow(d, 9));
    }
    else return 0;
}

float3 spiky_grad(float r, float3 r_vec) {
    if (r >= 0 && r <= d) {
        return -45 / (M_PI_F * pow(d, 6) * r) * pow(d - r, 2) * r_vec;
    }
    else return (float3){0.f, 0.f, 0.f};
}

float viscosity_laplace(float r) {
    if (r >= 0 && r <= d) {
        return 45 / (M_PI_F * pow(d, 5)) * (1 - r/d);
    }
    else {
        return 0;
    }
}


int boundaries(float3 position) {
    int toReturn = 0;
    if (position.x > xMax || position.x < 0) {
        toReturn |= CHANGE_X;
    }
    if (position.y < 0) {
        toReturn |= CHANGE_Y;
    }
    if (position.z > zMax || position.z < 0) {
        toReturn |= CHANGE_Z;
    }
    return toReturn;    
}


