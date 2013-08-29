int vec_add (int a, int b) {
    return (a + b) * 3;
}

__kernel void vector_add(__global int *A, __global int *B, __global int *C) {
    
    // Get the index of the current element
    int i = get_global_id(0);

    // Do the operation
    C[i] = vec_add (A[i], B[i]);
}
