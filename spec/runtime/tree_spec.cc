#include "runtime/runtime_spec_helper.h"
#include "runtime/tree.h"

START_TEST

enum {
    cat = 2,
    dog = 3,
    pig = 4,
};

static const char *names[] = { "error", "end", "cat", "dog", "pig" };

describe("Tree", []() {
  TSTree *tree1, *tree2, *parent1;

  before_each([&]() {
    tree1 = ts_tree_make_leaf(cat, 5, 2, 0);
    tree2 = ts_tree_make_leaf(cat, 3, 1, 0);
    parent1 = ts_tree_make_node(dog, 2, tree_array({ tree1, tree2, }), 0);
  });

  after_each([&]() {
    ts_tree_release(tree1);
    ts_tree_release(tree2);
    ts_tree_release(parent1);
  });

  describe("building a parent node", [&]() {
    it("computes its size based on its child nodes", [&]() {
      AssertThat(parent1->size, Equals<size_t>(9));
    });

    it("computes its padding based on its first child", [&]() {
      AssertThat(parent1->padding, Equals<size_t>(2));
    });

    it("computes the offset of each child node", [&]() {
      size_t count;
      TSTreeChild *children = ts_tree_visible_children(parent1, &count);

      AssertThat(count, Equals<size_t>(2));
      AssertThat(children[0].tree, Equals(tree1));
      AssertThat(children[0].offset, Equals<size_t>(0));
      AssertThat(children[1].tree, Equals(tree2));
      AssertThat(children[1].offset, Equals<size_t>(
          tree1->size + tree2->padding));
    });

    describe("when one of the child nodes is hidden", [&]() {
      TSTree *grandparent, *tree3;

      before_each([&]() {
        parent1->options = TSTreeOptionsHidden;
        tree3 = ts_tree_make_leaf(cat, 8, 5, 0);
        grandparent = ts_tree_make_node(pig, 2, tree_array({
            parent1,
            tree3,
        }), 0);
      });

      after_each([&]() {
        ts_tree_release(tree3);
        ts_tree_release(grandparent);
      });

      it("claims the hidden node's children as its own", [&]() {
        size_t count;
        TSTreeChild *children = ts_tree_visible_children(grandparent, &count);

        AssertThat(count, Equals<size_t>(3));
        AssertThat(children[0].tree, Equals(tree1));
        AssertThat(children[0].offset, Equals<size_t>(0));
        AssertThat(children[1].tree, Equals(tree2));
        AssertThat(children[1].offset, Equals<size_t>(
            tree1->size + tree2->padding));
        AssertThat(children[2].tree, Equals(tree3));
        AssertThat(children[2].offset, Equals<size_t>(
            tree1->size + tree2->padding + tree2->size + tree3->padding));
      });
    });
  });

  describe("equality", [&]() {
    it("returns true for identical trees", [&]() {
      TSTree *tree1_copy = ts_tree_make_leaf(cat, 5, 2, 0);
      AssertThat(ts_tree_equals(tree1, tree1_copy), Equals(1));
      TSTree *tree2_copy = ts_tree_make_leaf(cat, 3, 1, 0);
      AssertThat(ts_tree_equals(tree2, tree2_copy), Equals(1));

      TSTree *parent2 = ts_tree_make_node(dog, 2, tree_array({
          tree1_copy, tree2_copy,
      }), 0);
      AssertThat(ts_tree_equals(parent1, parent2), Equals(1));

      ts_tree_release(tree1_copy);
      ts_tree_release(tree2_copy);
      ts_tree_release(parent2);
    });

    it("returns false for trees with different symbols", [&]() {
      TSTree *different_tree = ts_tree_make_leaf(pig, 0, 0, 0);
      AssertThat(ts_tree_equals(tree1, different_tree), Equals(0));
      ts_tree_release(different_tree);
    });

    it("returns false for trees with different children", [&]() {
      TSTree *different_tree = ts_tree_make_leaf(pig, 0, 0, 0);
      TSTree *different_parent = ts_tree_make_node(dog, 2, tree_array({
          different_tree, different_tree,
      }), 0);

      AssertThat(ts_tree_equals(different_parent, parent1), Equals(0));
      AssertThat(ts_tree_equals(parent1, different_parent), Equals(0));

      ts_tree_release(different_tree);
      ts_tree_release(different_parent);
    });
  });

  describe("serialization", [&]() {
    it("returns a readable string", [&]() {
      char *string1 = ts_tree_string(tree1, names);
      AssertThat(string(string1), Equals("(cat)"));
      free(string1);

      char *string2 = ts_tree_string(parent1, names);
      AssertThat(string(string2), Equals("(dog (cat) (cat))"));
      free(string2);
    });

    it("hides invisible nodes", [&]() {
      tree2->options = TSTreeOptionsHidden;

      char *string1 = ts_tree_string(parent1, names);
      AssertThat(string(string1), Equals("(dog (cat))"));
      free(string1);
    });

    describe("when the root node is not visible", [&]() {
      it("still serializes it", [&]() {
        parent1->options = TSTreeOptionsHidden;

        char *string1 = ts_tree_string(parent1, names);
        AssertThat(string(string1), Equals("(dog (cat) (cat))"));
        free(string1);

        tree1->options = TSTreeOptionsHidden;

        char *string2 = ts_tree_string(tree1, names);
        AssertThat(string(string2), Equals("(cat)"));
        free(string2);
      });
    });
  });
});

END_TEST
