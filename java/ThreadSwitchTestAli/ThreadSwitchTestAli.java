// wget https://repo1.maven.org/maven2/net/openhft/affinity/3.23.3/affinity-3.23.3.jar
// javac -cp "./*:." ThreadSwitchTestAli.java
// java -verbose -cp "./*:." -Dthread.switch.test.warmup.iter=1234567 -Dthread.switch.test.run.iter=1234567 ThreadSwitchTestAli 7 1 abc

//package com.arm;
import net.openhft.affinity.Affinity;
import net.openhft.affinity.AffinityLock;
//import org.apache.commons.lang.ArrayUtils;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.BitSet;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import static java.util.concurrent.Executors.newFixedThreadPool;
public class ThreadSwitchTestAli {
    public static Integer synchronizedCount = 0;
    public static Long synchronizedCost = 0L;
    public static Long synchronizedStart = 0L;
    public static ExecutorService excutorService;
    public static ExecutorService bizExcutors;
    int consumerNum = 1;
    int providerNum = 1;
    Map<Long, Long> excuteState;
    Map<Long, List<Long>> totalCosts;
    Integer pCpuId = 0;
    Integer cCpuId = 1;
    Integer runIter = Integer.valueOf(System.getProperty("thread.switch.test.run.iter", "1000000")) / providerNum;
    Boolean runSync = Boolean.valueOf(System.getProperty("thread.switch.test.run.sync", "true"));
    Integer warmUpIter = Integer.valueOf(System.getProperty("thread.switch.test.warmup.iter", "100000")) / providerNum;
    ThreadSwitchTestAli(int providers, int consumers){
        consumerNum = consumers;
        providerNum = providers;
        bizExcutors = newFixedThreadPool(providerNum);
        excutorService = newFixedThreadPool(consumerNum);
        excuteState = new HashMap<>();
        totalCosts = new HashMap<>();
        parseCPU();
    }
    void parseCPU(){
        String strProducerCpu = System.getProperty("thread.switch.test.producer.vcpu", null);
        String strConsumerCpu = System.getProperty("thread.switch.test.consumer.vcpu", null);
        if(strProducerCpu != null) {
            pCpuId = Integer.valueOf(strProducerCpu);
        }
        if(strConsumerCpu != null) {
            cCpuId = Integer.valueOf(strConsumerCpu);
        }
    }
    List parseCPUSet(String cpuSetStr){
        List<Integer> cpuRawSet = new ArrayList<>();
        if(cpuSetStr.indexOf('-') != -1){
            Integer begin = Integer.valueOf(cpuSetStr.split("-")[0]);
            Integer end = Integer.valueOf(cpuSetStr.split("-")[1]);
            for(Integer i=begin;i<=end;i++){
                cpuRawSet.add(i);
            }
        }else{
            for(String s :cpuSetStr.split(",")){
                cpuRawSet.add(Integer.valueOf(s));
            }
        }
        return cpuRawSet;
    }
    void calResult(){
        float totalWarmUpCosts = 0f;
        float totalRunCosts = 0L;
        float totalRunCount = 0L;
        for(Long id : excuteState.keySet()){
            List<Long> warmUpSet = totalCosts.get(id).subList(0, warmUpIter);
            List<Long> normalSet = totalCosts.get(id).subList(1000, (excuteState.get(id)).intValue() - warmUpIter);
            totalWarmUpCosts += warmUpSet.stream().mapToDouble(a->a).sum();
            totalRunCosts += normalSet.stream().mapToDouble(a->a).sum();
            totalRunCount += excuteState.get(id);
//                      System.out.println("thread: " + id + ", " + totalCosts.get(id).size());
            System.out.println("thread: " + id + ", execute: " + excuteState.get(id) +
                    ", warmup avg:" + warmUpSet.stream().mapToDouble(a -> a).average().getAsDouble() +
                    ", " + " avg: " + normalSet.stream().mapToDouble(a -> a).average().getAsDouble());
        }
        System.out.println("total execute: " + totalRunCount +
                ", warmup avg:" + totalWarmUpCosts/totalRunCount +
                ", " + "normal avg: " + totalRunCosts/totalRunCount);
        if(synchronizedCount>0) {
            System.out.println("total synchronized: " + synchronizedCount + "; total synchronized cost: " + synchronizedCost + "; avg: " + synchronizedCost / synchronizedCount);
        }
    }
    public void runByIter(){
        for(int i=0; i<providerNum; i++) {
            ProviderThreadJob job = new ProviderThreadJob();
            job.setType(1);
            bizExcutors.execute(job);
        }
        bizExcutors.shutdown();
        try{
            bizExcutors.awaitTermination(Long.MAX_VALUE, TimeUnit.SECONDS);
        }catch (InterruptedException e){
            System.out.println(e);
        }
        excutorService.shutdown();
        try{
            excutorService.awaitTermination(Long.MAX_VALUE, TimeUnit.MILLISECONDS);
        }catch (InterruptedException e){
            System.out.println(e);
        }
        calResult();
    }
    public class ProviderThreadJob implements Runnable {
        int jobType = 1;
        public void setType(int type){
            jobType = type;
        }
        public void runByIter(){
            Long st = System.currentTimeMillis();
            Long execIter = 0L;
            Integer totalIter = warmUpIter + runIter;
            while(totalIter > execIter) {
                ConsumerThreadJob job = new ConsumerThreadJob();
                if(runSync){
                    Future future = excutorService.submit(job);
                    try {
                        future.get();
                    }catch (Exception e){
                        System.out.println("got an exception." + e.getMessage());
                    }
                }
                else {
                    excutorService.execute(job);
                }
                execIter ++;
            }
            System.out.println("submit time: " + (System.currentTimeMillis() - st));
        }
        @Override
        public void run(){
            runByIter();
            System.out.println(Thread.currentThread() + " quit.");
        }
    }
    public class ConsumerThreadJob implements Runnable {
        Long startTime = 0L;
        Long costs = 0L;
//        UUID id = UUID.randomUUID();
        Lock lock;
        Boolean isWarmUp;
        ConsumerThreadJob(){
            lock = new ReentrantLock();
            isWarmUp = false;
            reset();
        }
//        public UUID getId(){
//            return id;
//        }
        void reset(){
            startTime = System.nanoTime();
            costs = 0L;
        }
        public void setIsWarmUp(boolean isWarmUp){
            this.isWarmUp = isWarmUp;
        }
        @Override
        public void run() {
            costs = (System.nanoTime() - startTime);
            Long id = Thread.currentThread().getId();
//            lock.lock();
////            synchronized (synchronizedCount) {
//                synchronizedCount++;
//                synchronizedStart = System.nanoTime();
                try {
                    synchronized (totalCosts) {
                        if (!totalCosts.containsKey(id)) {
                            totalCosts.put(id, new ArrayList<>());
                        }
                        totalCosts.get(id).add(costs);
                        if (excuteState.containsKey(id)) {
                            excuteState.put(id, excuteState.get(id) + 1L);
                        } else {
                            excuteState.put(id, 1L);
                        }
                    }
                    synchronizedCost += System.nanoTime() - synchronizedStart;
                } finally {
//                    lock.unlock();
                }
//            }
        }
    }
    public static void main(String[] args) throws InterruptedException {
        long start = System.currentTimeMillis();
        int providerNum = Integer.parseInt(args[0]);
        int consumerNum = Integer.parseInt(args[1]);
        String caseName = args[2];
        ThreadSwitchTestAli testAgent = new ThreadSwitchTestAli(providerNum, consumerNum);
        testAgent.runByIter();
        System.out.println("The total runtime of the program is: " + (System.currentTimeMillis() - start) + " ms" );
    }
}

