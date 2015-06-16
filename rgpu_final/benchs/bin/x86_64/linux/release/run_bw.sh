##Benchmarking for rCUDA

NR_ITER=10

touch results.out
echo "Running NVIDIA benchmarks for rCUDA"


##BandWidth Test htd, dth
echo 'Running Bandwidth Test...'
echo 'Bandwidth Test (32 MB)' >> results.out
echo '------------- ' >> results.out

for i in `seq 1 $NR_ITER`
do
	./bandwidthTest | awk '{if ((NR==10)||(NR==15) || (NR==20)) print $2}' > tmp
	HTD=`cat tmp | awk '{if (NR==1) print $1}'`
	DTH=`cat tmp | awk '{if (NR==2) print $1}'`
	DTD=`cat tmp | awk '{if (NR==3) print $1}'`
	echo $HTD $DTH $DTD>> results.out
done

rm -f tmp
echo 'Done'
echo '----------------------' >> results.out

