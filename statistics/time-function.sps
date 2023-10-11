COMPUTE log_branches = lg10(base_branches).
COMPUTE log_time = lg10(base_time).
COMPUTE log_tree = lg10(base_treemoves).
temporary.
SELECT IF (base_treemoves <= 75000).
GRAPH SCATTERPLOT(BIVARIATE) = base_treemoves WITH base_time BY base_outcome.
