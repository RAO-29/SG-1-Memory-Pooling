# Uncomment the following lines, if not yet installed
install.packages('rstudioapi')
install.packages('ggplot2')

library('ggplot2')
library('rstudioapi')
library('plyr')
library('scales')

data_file_clib_magic = "results-bench-mem-pools-mempool-magic.csv"
data_file_clib_mempool = "results-bench-mem-pools-mempool-none.csv"

result_dir = paste(dirname(rstudioapi::getActiveDocumentContext()$path), sep = "", "/../results");

data_file_path_clib_magic   = paste(result_dir, sep = "/", data_file_clib_magic)
data_file_path_clib_mempool = paste(result_dir, sep = "/", data_file_clib_mempool)

data_clib_magic = read.csv(file=data_file_path_clib_magic, head = TRUE, sep = ",")
data_clib_mempool = read.csv(file=data_file_path_clib_mempool, head = TRUE, sep = ",")

data_clib_magic_agg = ddply(data_clib_magic,~alpha,summarise,mean=mean(1000/call_duration_ms),sd=sd(1000/call_duration_ms))
data_clib_mempool_agg = ddply(data_clib_mempool,~alpha,summarise,mean=mean(1000/call_duration_ms),sd=sd(1000/call_duration_ms))

plot(data_clib_magic_agg$mean)
plot(data_clib_mempool_agg$mean)

df = data.frame(
    ops_per_sec = c(data_clib_magic_agg$mean, data_clib_mempool_agg$mean),
    sd = c(data_clib_magic_agg$sd, data_clib_mempool_agg$sd),
    ratio = c(data_clib_magic_agg$alpha, data_clib_mempool_agg$alpha),
    label = c(rep("mempool (improved)", length(data_clib_magic_agg$mean)), rep("mempool (trivial)  ", length(data_clib_mempool_agg$mean)))
)

breaks = round(seq(min(df$ops_per_sec),max(df$ops_per_sec), length.out = 5))

ggplot(data=df, aes(x=ratio, y=ops_per_sec, group=label)) + 
  geom_errorbar(aes(x = ratio, ymin=ops_per_sec - sd, ymax=ops_per_sec + sd, colour = label), alpha = 0.4) + 
  geom_point(aes(shape=label, colour = label), size=2.0) +
  geom_line(aes(linetype=label, colour = label), size=1.0) +
  scale_color_brewer(palette="Paired")+
  theme_minimal() + xlab("") + 
  theme(plot.margin=unit(c(1,1,1.5,1.2),"cm"), plot.title = element_text(size=12), legend.position="top", legend.title=element_blank(), legend.justification = c(0, 0),
        legend.direction = "horizontal", axis.title.y=element_blank(), axis.title.x=element_text(size=10)) + scale_y_continuous(breaks=breaks, labels = unit_format(unit = "mio ops/sec", scale = 1e-6, sep = " ", digits = 0)) +
  scale_x_continuous(breaks=c(0,0.25,0.50,0.75,1.0), labels=c("free\nonly", "75% free\n25% realloc", "50% free\n50% realloc", "25% free\n75% realloc", "realloc\nonly")) +
  ggtitle("Memory Pool Performance Improvements")