extern int input(void);
extern void output(int);

typedef struct TreeNode {
  int value;
  struct TreeNode *left, *right;
} TreeNode;

extern TreeNode* tn_malloc(int);

int sum(TreeNode* root) {
  if (root == 0) return 0;
  return root->value + sum(root->left) + sum(root->right);
}

TreeNode* construct(int value, TreeNode* left, TreeNode* right) {
  TreeNode* root = tn_malloc(1);
  root->value = value;
  root->left = left;
  root->right = right;
  return root;
}

int main() {
  TreeNode* left = construct(1, 0, 0);
  TreeNode* right = construct(3, 0, 0);
  TreeNode* root = construct(2, left, right);
  output(sum(root));
}
