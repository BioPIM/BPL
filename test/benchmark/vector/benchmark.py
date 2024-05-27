#!/usr/bin/python3

import sys
import os
import pandas as pd
import plotly.express as px

# input looks like:
# log2(items)   vector    handcrafted   upmem
#           4   2.20869   0.19036     7.02563
#           5   2.71652   0.38071     6.99051

df = pd.read_csv (sys.argv[1], sep='\s+');

nbName   = df.columns[0]
rateName = "rate (millions / sec)";

# -> we need to put it in the following form
#  4        vector   0.36619
#  4   handcrafted   0.19036
#  4         upmem   7.15752
#  5        vector   0.65680
#  5   handcrafted   0.38075
#  5         upmem   7.01389

def build(df,name):
    res = df[[nbName,name]];   
    res.insert (2,'type', name)
    res.columns = [nbName, rateName, "type"]
    return res;

df = pd.concat([
    build (df,"vector"),
    build (df,"handcrafted"),
    build (df,"upmem")
]);
df.reset_index()

title = sys.argv[2] if len(sys.argv)>=3 else "";

fig = px.line(df, title=title, x=df[nbName], y=df[rateName], color=df["type"], markers=True)

fig.update_layout (title=dict(font=dict(size=30)))

fig.update_layout (yaxis_range=[0,7])

fig.show()

pngfile = ".".join (sys.argv[1].split(".")[0:-1])
fig.write_image ("{:s}.png".format(pngfile), width=1980, height=1080);

