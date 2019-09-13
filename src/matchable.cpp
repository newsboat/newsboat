bool matchable_has_attribute(void *ptr, char *attr) {
	Matchable *matchable = (Matchable*) ptr;
	return matchable->has_attribute(attr);
}

char* matchable_get_attribute(void *ptr, char *attr) {
	Matchable *matchable = (Matchable*) ptr;
	return matchable->get_attribute(attr);
}
