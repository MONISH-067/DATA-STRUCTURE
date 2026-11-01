def binary_search(arr, tar):
    low, high = 0, len(arr) - 1

    for i, _ in enumerate(arr):
        mid = (low + high) // 2
        if mid == tar:
            return 
