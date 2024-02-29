from networkx.classes.coreviews import AdjacencyView as _AdjacencyView
from networkx.classes.coreviews import AtlasView as _AtlasView


class AtlasView(_AtlasView):
    def __eq__(self, other):
        return self._atlas.__eq__(other)

    def __contains__(self, key):
        return key in self._atlas


class AdjacencyView(AtlasView):
    __slots__ = ()  # Still uses AtlasView slots names _atlas

    def __getitem__(self, name):
        return AtlasView(self._atlas[name])
