bst_values = [0x24, 0x8, 0x32, 0x6, 0x16, 0x2d, 0x6b, 0x1, 0x7, 0x14, 0x23, 0x28, 0x2f, 0x63, 0x3e9]
bst_node  = [[1, 2], [3, 4], [5, 6], [7, 8], [9, 10], [11, 12], [13, 14], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1], [-1, -1]]

def fun7(search_value : int, target_node : int) -> int:
    if (target_node == -1):
        return -1
    if (bst_values[target_node] > search_value):
        return 2 * fun7(search_value, bst_node[target_node][0])
    result = 0
    if (bst_values[target_node] != search_value) :
        return 2 * fun7(search_value, bst_node[target_node][1]) + 1
    return result

for i in range(0x400):
    result = fun7(i, 0)
    if result == 0x2:
        print(i, hex(i))