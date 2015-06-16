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
	./bandwidthTest | awk '{if ((NR==10)||(NR==15)) print $2}' > tmp
	HTD=`cat tmp | awk '{if (NR==1) print $1}'`
	DTH=`cat tmp | awk '{if (NR==2) print $1}'`
	echo $HTD $DTH >> results.out
done

rm -f tmp
echo 'Done'
echo '----------------------' >> results.out


##MergeSort Test
#echo 'Running MergeSort Test...'
#echo 'MergeSort' >> results.out
#echo '------------- ' >> results.out


#for i in `seq 1 $NR_ITER`
#do
#	./mergeSort | awk '{if (NR==11) print $2}' >> results.out
#done
#echo 'Done'
#echo '----------------------' >> results.out


##Binomial Options Test
echo 'Running Binomial Options Test'
echo 'Binomial Options' >> results.out
echo '---------------- ' >> results.out


for i in `seq 1 $NR_ITER`
do
	./binomialOptions | awk '{if (NR==9) print $3}' >> results.out
done
echo 'Done'
echo '----------------------' >> results.out


##Scalar Product Test
echo 'Running Scalar Product Test...'
echo 'Scalar Product' >> results.out
echo '-------------- ' >> results.out


for i in `seq 1 $NR_ITER`
do
	./scalarProd | awk '{if (NR==12) print $3}' >> results.out
done
echo 'Done'
echo '----------------------' >> results.out


##Matrix Transpose Test
echo 'Running Matrix Transpose Test...'
echo 'Matrix Transpose' >> results.out
echo '----------------' >> results.out


for i in `seq 1 $NR_ITER`
do
	./transpose -dimX=8192 -dimY=8192 | awk '{if (NR==18) print $10}' >> results.out
done
echo 'Done'
echo '----------------------' >> results.out
