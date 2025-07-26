#include "B_Plus_Tree.h"
int Node::lower_bound(int key) {
    int left = -1, right = get_size();
    while (left + 1 < right) {
        int mid = (left + right) / 2;
        if (keys[mid] >= key) {
            right = mid;
        } else {
            left = mid;
        }
    }
    return right;
}
int Node::upper_bound(int key) {
    int left = -1, right = get_size();
    while (left + 1 < right) {
        int mid = (left + right) / 2;
        if (keys[mid] > key) {
            right = mid;
        } else {
            left = mid;
        }
    }
    return right;
}
int Node::internal_lookup(int key) {
    return max(upper_bound(key) - 1, 0);
}

bool Node::leaf_lookup(int key){
	int pos = lower_bound(key);
	if(pos < get_size() && keys[pos] == key){
		return true;
	}else{
		return false;
	}
}
void Node::insert_pair(int pos, int key, Node *node){
	keys.insert(keys.begin() + pos, key);
	children.insert(children.begin() + pos, node);
}
void Node::erase_pair(int pos){
	keys.erase(keys.begin() + pos);
	children.erase(children.begin() + pos);
}
int Node::get_size(){
	assert(keys.size() == children.size());
	return keys.size();
}
int Node::get_min_size(){
	return (maxLen + 1) / 2;
}
int Node::get_max_size(){
	return maxLen;
}
int Node::find_child(Node *node){
	for(int i = 0; i < children.size(); i++){
		if(children[i] == node){
			return i;
		}
	}
	assert(0);
	return 0;
}
void BPlusTree::insert_entry(int key) {
    Node* leaf = find_leaf_node(key);
	if(!leaf->leaf_lookup(key)){
		int pos = leaf->lower_bound(key);
		leaf->insert_pair(pos, key, nullptr);
		if (leaf->get_size() == leaf->get_max_size()) {
			Node* newLeaf = split(leaf);
			insert_into_parent(leaf, newLeaf);
		}
		maintain_parent(leaf);
	}

}
Node* BPlusTree::find_leaf_node(int key) {
    Node* node = root;
    while (!node->isLeaf) {
        Node* new_node = node->children[node->internal_lookup(key)];
        node = new_node;
    }
    return node;
}
Node* BPlusTree::split(Node* old_node) {
    Node* new_node = new Node(old_node->isLeaf);
    new_node->parent = old_node->parent;
    if (old_node->isLeaf) {
        new_node->next = old_node->next;
        new_node->prev = old_node;
        old_node->next = new_node;
        if (new_node->next) {
            new_node->next->prev = new_node;
        }
    }
    int pos = old_node->get_size() / 2;
    int num = old_node->get_size() - pos;
    for(int i = 0; i < num; i++){
    	new_node->insert_pair(i, old_node->keys[i + pos], old_node->children[i + pos]);
	}
	for(int i = 0; i < num; i++){
		old_node->erase_pair(old_node->get_size() - 1);
	}
    if (!old_node->isLeaf) {
        for (int i = 0; i < num; i++) {
            maintain_child(new_node, i);
        }
    }
    return new_node;
}
void BPlusTree::insert_into_parent(Node* old_node, Node* new_node) {
    if (old_node == root) {
        Node* new_root = new Node(false);
        new_root->insert_pair(0, old_node->keys.front(), old_node);
        new_root->insert_pair(1, new_node->keys.front(), new_node);
        old_node->parent = new_root;
        new_node->parent = new_root;
        root = new_root;
    } else {
        Node* old_parent = old_node->parent;
        int pos = old_parent->find_child(old_node);
        old_parent->insert_pair(pos + 1, new_node->keys.front(), new_node);
        if (old_parent->get_size() == old_parent->get_max_size()) {
            Node* new_parent = split(old_parent);
            insert_into_parent(old_parent, new_parent);
        }
    }
}
void BPlusTree::maintain_child(Node* node, int idx) {
	node->children[idx]->parent = node;
}
void BPlusTree::maintain_parent(Node* node){
	while(node->parent){
		Node *parent = node->parent;
    	int pos = parent->find_child(node);
    	if(node->keys.front() == parent->keys[pos]){
			break;
		}
		parent->keys[pos] = node->keys.front();
    	node = parent;
	}
}
    
bool BPlusTree::delete_entry(int key) {
	Node *leaf = find_leaf_node(key);
	if(leaf->leaf_lookup(key)){
		int pos = leaf->lower_bound(key);
		leaf->erase_pair(pos);
		if(leaf->get_size() < leaf->get_min_size()){
			return coalesce_or_redistribute(leaf);
		}else{
			maintain_parent(leaf);
			return false;
		}
	}
	return false;
}
bool BPlusTree::coalesce_or_redistribute(Node* node) {
	if (node == root) {
		return adjust_root(node);
	} else if (node->get_size() >= node->get_min_size()) {
		maintain_parent(node);
		return false;
	}
	Node *parent = node->parent;
	int pos = parent->find_child(node);
	Node *neighbor_node = (pos == 0) ? parent->children[1] : parent->children[pos - 1];
	if (node->get_size() + neighbor_node->get_size() >= node->get_min_size() * 2) {
		redistribute(neighbor_node, node, pos);
		return false;
	} else {
		coalesce(neighbor_node, node, parent, pos);
		return true;
	}
	return false;
}
bool BPlusTree::adjust_root(Node* old_root) {
	if (!old_root->isLeaf && old_root->get_size() == 1) {
		Node *new_root = old_root->children.front();
		new_root->parent = nullptr;
		delete root;
		root = new_root;
		return true;
	} else if (old_root->isLeaf && old_root->get_size() == 0) {
		return true;
	}
	return false;
}
void BPlusTree::redistribute(Node* neighbor_node, Node* node, int pos) {
	if (pos == 0) {
		node->insert_pair(node->get_size(),
		neighbor_node->keys.front(),
		neighbor_node->children.front());
		neighbor_node->erase_pair(0);
		maintain_parent(neighbor_node);
		if(!node->isLeaf){
			maintain_child(node, node->get_size() - 1);
		}
	} else {
		node->insert_pair(0,
		neighbor_node->keys.back(),
		neighbor_node->children.back());
		neighbor_node->erase_pair(neighbor_node->get_size() - 1);
		maintain_parent(node);
		if(!node->isLeaf){
			maintain_child(node, 0);
		}
	}
}
bool BPlusTree::coalesce(Node* neighbor_node, Node* node, Node* parent, int pos) {
	if (pos == 0) {
		swap(node, neighbor_node);
		pos++;
	}
	for (int i = 0; i < node->get_size(); i++) {
		neighbor_node->insert_pair(neighbor_node->get_size(), node->keys[i], node->children[i]);
		if(!neighbor_node->isLeaf){
			maintain_child(neighbor_node, neighbor_node->get_size() - 1);
		}
	}
	erase_leaf(node);
	delete node;
	parent->erase_pair(pos);
	maintain_parent(neighbor_node);
	return coalesce_or_redistribute(parent);
}
void BPlusTree::erase_leaf(Node* leaf) {
	if(leaf->prev){
	    Node* prev = leaf->prev;
	    prev->next = leaf->next;
	}
	if(leaf->next){
	    Node* next = leaf->next;
	    next->prev = leaf->prev;
	}
}
void BPlusTree::printLevelOrder() {
    queue<Node*> q;
    q.push(root);
    int level = 0;
    while (!q.empty()) {
        int size = q.size();
        cout << "Level " << level++ << ": ";
        for (int i = 0; i < size; ++i) {
            Node* node = q.front();
            q.pop();
            cout << "[";
            for (size_t j = 0; j < node->keys.size(); ++j) {
                cout << node->keys[j];
                if (j < node->keys.size() - 1) cout << ", ";
            }
            cout << "] ";
            if (!node->isLeaf) {
                for (auto child : node->children) {
                    q.push(child);
                }
            }
        }
        cout << endl;
    }
}
bool BPlusTree::count(int key) {
    Node* leaf = find_leaf_node(key);
    return leaf->leaf_lookup(key);
}
